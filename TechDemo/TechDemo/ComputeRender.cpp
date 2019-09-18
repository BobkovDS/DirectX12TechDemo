#include "ComputeRender.h"


ComputeRender::ComputeRender() : m_textureID(0), m_bufferFlag(true), m_isReady(false)
{
	m_drop = false;
}

ComputeRender::~ComputeRender()
{
}

void ComputeRender::initialize(const RenderMessager& renderParams)
{
	RenderBase::initialize(renderParams);
}


void ComputeRender::build(int ObjectID)
{
	assert(m_initialized == true);

	m_ObjectID = ObjectID;

	Scene::SceneLayer* lObjectLayer = nullptr;
	lObjectLayer = m_scene->getLayer(NOTOPAQUELAYERCH);
	
	if (lObjectLayer)
	{
		Scene::SceneLayer::SceneLayerObject* lObject = lObjectLayer->getSceneObject(ObjectID);
		if (lObject == nullptr) return; // Layer does not have requested Object

		const InstanceDataGPU* lpInstance = &lObject->getObjectMesh()->Instances[0];
		if (lpInstance == nullptr) return; // object does not have Instances

		int lMaterial = lpInstance->MaterialIndex;
		MaterialCPU* lpMaterial = m_resourceManager->getMaterial(lMaterial);
		if (lpMaterial == nullptr) return; // Instance does not have material

		UINT lWidth = lpMaterial->WaterV2_Width;
		UINT lHeight = lpMaterial->WaterV2_Height;
		
		m_width = lWidth; 
		m_height = lHeight;
		m_textureID = lpMaterial->DiffuseColorTextureIDs[0];

		/* 1. Create Textures with Input Data. Actually now here no data. But it can be some input High values.
			Later the compute shader will fill it with:
			Float4.x = Vertex high value
			float4.yzw = Normal for this vertex
		*/

		UINT lVerticesCount = m_width * m_height;
		std::vector<DirectX::XMFLOAT4> lVerticesData(lVerticesCount);

		for (int i = 0; i < lVerticesCount; i++)			
			lVerticesData[i] = DirectX::XMFLOAT4(0.0f, 0.0f, 0, 0.0f);				

		m_inputResource.ResourceInDefaultHeap = Utilit3D::createTextureWithData(m_device, m_cmdList,
			lVerticesData.data(), sizeof(DirectX::XMFLOAT4), m_width, m_height,
			DXGI_FORMAT_R32G32B32A32_FLOAT, m_inputResource.ResourceInUploadHeap, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		// 2. Create two work Textures for Compute Shader
		create_ResrourcesUAV(DXGI_FORMAT_R32G32B32A32_FLOAT);

		// 3. Copy input Data to just new created two Resources
		m_uavResources[0]->changeState(m_cmdList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		m_uavResources[1]->changeState(m_cmdList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_inputResource.ResourceInDefaultHeap.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE));

		m_cmdList->CopyResource(m_uavResources[0]->getResource(), m_inputResource.ResourceInDefaultHeap.Get());
		m_cmdList->CopyResource(m_uavResources[1]->getResource(), m_inputResource.ResourceInDefaultHeap.Get());

		m_uavResources[0]->changeState(m_cmdList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_uavResources[1]->changeState(m_cmdList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_inputResource.ResourceInDefaultHeap.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ));

		// Initialize PSO layer
		m_psoLayer.buildPSO(m_device, m_rtResourceFormat, m_dsResourceFormat);

		build_TechDescriptors();
		//m_timer.setTickTime(0.008333); //120fps;
		m_timer.setTickTime(0.029f); //60fps; 0.029f

		m_isReady = true;
	}
}

void ComputeRender::create_ResrourcesUAV(DXGI_FORMAT resourceFormat)
{
	m_own_resources.resize(2);

	// "A" Resource
	m_uavResources.push_back(create_Resource(resourceFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
	// "B" Resource
	m_uavResources.push_back(create_Resource(resourceFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
}

void ComputeRender::create_DescriptorHeap_Compute()
{
	// to store Compute Descriptor Heap we will use m_own_dsvHeap member
	D3D12_DESCRIPTOR_HEAP_DESC lHeapDesc = {};
	lHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	lHeapDesc.NumDescriptors = 3;
	lHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT res = m_device->CreateDescriptorHeap(&lHeapDesc, IID_PPV_ARGS(&m_own_dsvHeap));
	assert(SUCCEEDED(res));

	m_descriptorHeapCompute = m_own_dsvHeap.Get();	
}

void ComputeRender::build_TechDescriptors()
{
	/*
		We have to Descriptors heap:
			- m_descriptorHeap. It is common application's descirptor heap for graphic draw. We need it to create a SRV
			(for m_inputResource) for FinalRender_CH_shaders vertex shader, because this graphic draw will use this SRV
			to draw WaterV2 object with Height values from corresponding texture.

			-m_descriptorHeapCompute. it is local descriptor heap for compute shader work. In this descriptor heap we create 5 descriptors:
				- UAV for m_inputResource to save compute result in result texture;
				- 2 SRV and 2 UAV for intermediate textures (m_uavResources[2]) for compute work.
	*/

	UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	// 1. For Graphic call in other part of application. 
	//		Create SRV on input Texture (it is a result texture for computing in the same time)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptor(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart());
		lhDescriptor.Offset(TECHSRVCOUNT, lSrvSize);
		lhDescriptor.Offset(m_textureID, lSrvSize);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

		// SRV for ViewNormal Map
		m_device->CreateShaderResourceView(m_inputResource.ResourceInDefaultHeap.Get(), NULL, lhDescriptor);
	}

	// 2. For Compute work in this class
	//		Create SRV/UAVs for both intermediate resources and 1 UAV for result texture (input texture)
	{
		// 2.a Create Descriptor heaps
		create_DescriptorHeap_Compute();
		CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptor(m_descriptorHeapCompute->GetCPUDescriptorHandleForHeapStart());

		// 2.b Create Descriptors in Descriptor heap

		// 2.b.1 Create UAV for Result Texture
		m_device->CreateUnorderedAccessView(m_inputResource.ResourceInDefaultHeap.Get(), nullptr, nullptr, lhDescriptor);

		// 2.b.2 Create 2UAV for m_uavResources[] resources				
		// CurrentBuffer texture - UAV
		lhDescriptor.Offset(1, lSrvSize);
		m_device->CreateUnorderedAccessView(m_uavResources[0]->getResource(), nullptr, nullptr, lhDescriptor);		
		// PrevBuffer texture - UAV
		lhDescriptor.Offset(1, lSrvSize);
		m_device->CreateUnorderedAccessView(m_uavResources[1]->getResource(), nullptr, nullptr, lhDescriptor);
	}	
}

void ComputeRender::draw(int flags)
{
	if (!m_isReady) return;

	if (m_timer.tick())
	{
		int lXPos= 0;
		int lYPos= 0;
		int lMagnitude= 0;

		if (m_drop)
		{
			lXPos = (double)rand() / (RAND_MAX + 1.0f) * m_width;
			lYPos = (double)rand() / (RAND_MAX + 1.0f) * m_height;
			//lXPos = (double)rand() / (RAND_MAX + 1) * 10 + m_width/2;
			//lYPos = (double)rand() / (RAND_MAX + 1) * 10;
			lMagnitude = (double) rand() / (RAND_MAX + 1) * 50 + 10;
			//lXPos = m_width / 2;
			//lYPos = 2;
			m_drop = false;
		}

		m_cmdList->SetComputeRootSignature(m_psoLayer.getRootSignature());
		m_cmdList->SetPipelineState(m_psoLayer.getPSO(OPAQUELAYER));

		// Set DescriptorHeaps  
		ID3D12DescriptorHeap* ldescriptorHeaps[] = { m_descriptorHeapCompute };
		m_cmdList->SetDescriptorHeaps(1, ldescriptorHeaps);

		// Set RootArguments	
		auto passCB = m_frameResourceManager->getCurrentSSAOCBResource();
		m_cmdList->SetComputeRootConstantBufferView(2, passCB->GetGPUVirtualAddress()); // Pass constant buffer data

		CD3DX12_GPU_DESCRIPTOR_HANDLE lhGpuDescriptor(m_descriptorHeapCompute->GetGPUDescriptorHandleForHeapStart());
		m_cmdList->SetComputeRootDescriptorTable(1, lhGpuDescriptor);
		m_cmdList->SetComputeRoot32BitConstant(0, lXPos, 0); // Tech Flags: Even Pass (TRUE)			
		m_cmdList->SetComputeRoot32BitConstant(0, lYPos, 1); // Tech Flags: Even Pass (TRUE)			
		m_cmdList->SetComputeRoot32BitConstant(0, lMagnitude, 2); // Tech Flags: Even Pass (TRUE)			

		// Compute call. 
	//	m_height = 280;
		UINT numGroups = (UINT)ceilf(m_height / 256.0f);
		m_cmdList->Dispatch(1, numGroups, 1);

		m_bufferFlag = !m_bufferFlag;
	}
}

void ComputeRender::drop()
{
	m_drop = true;
}