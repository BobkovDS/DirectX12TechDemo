#include "BlurRender.h"

using namespace DirectX;

BlurRender::BlurRender()
{
}

BlurRender::~BlurRender()
{
}

void BlurRender::initialize(const RenderMessager& renderParams)
{
	RenderBase::initialize(renderParams);
}

void BlurRender::setInputResource(ID3D12Resource* inputResource)
{
	m_inputResource = inputResource;
}

void BlurRender::build(std::string shaderName, int blurCount)
{
	assert(m_initialized == true);

	m_blurCount = blurCount;
	if (m_blurCount % 2 != 0)
		m_blurCount++; // make it even, that the last draw call was back to inputResource

	m_own_resources.resize(1);

	// Render Target Resource. This class own it
	{
		create_Resource_RT(m_rtResourceFormat); // for result/work Blur		
		create_DescriptorHeap_RTV();
	}

	// Initialize PSO layer
	m_psoLayer.buildPSO(m_device, m_rtResourceFormat, m_dsResourceFormat, shaderName);

	// Initialize DescriptorHandle: Tech_DescriptorHandle
	m_techSRVHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_GPU_DESCRIPTOR_HANDLE lhDescriptor(m_techSRVHandle);
	lhDescriptor.Offset(TECHSLOT_SSAO_VSN, lSrvSize);
	m_techSRVHandle = lhDescriptor;

	build_screen();
	build_TechDescriptors();	
}

void BlurRender::create_DescriptorHeap_RTV()
{
	// Create Descriptor heap for RenderTargetViews
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT res = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_own_rtvHeap));
	assert(SUCCEEDED(res));

	m_rtvHeap = m_own_rtvHeap.Get();
	m_rtvHeapWasSetBefore = false;
}

void BlurRender::build_TechDescriptors()
{
	// Create RTVs for both resources (Input Resource and One "our" Resource)
	{
		UINT lRtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_cpu_RTV_Handle1 = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		m_cpu_RTV_Handle2 = m_cpu_RTV_Handle1;
		m_cpu_RTV_Handle2.ptr += lRtvDescriptorSize;

		m_device->CreateRenderTargetView(m_inputResource, nullptr, m_cpu_RTV_Handle1);
		m_device->CreateRenderTargetView(m_rtResources[0]->getResource(), nullptr, m_cpu_RTV_Handle2);
	}

	// Create SRVs for both resources (Input Resource and One "our" Resource)
	{
		UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptor(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptorOffseting;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

		lhDescriptorOffseting = lhDescriptor;
		lhDescriptorOffseting.Offset(TECHSLOT_SSAO_AO, lSrvSize);
		m_device->CreateShaderResourceView(m_inputResource, nullptr, lhDescriptorOffseting);// actually SSAORender already has created SRV for this
	
		lhDescriptorOffseting = lhDescriptor;
		lhDescriptorOffseting.Offset(TECHSLOT_SSAO_BLUR, lSrvSize);
		m_device->CreateShaderResourceView(m_rtResources[0]->getResource(), nullptr, lhDescriptorOffseting);

		CD3DX12_GPU_DESCRIPTOR_HANDLE lhGpuDescriptor(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE lhGpuDescriptorOffseting;
		lhGpuDescriptorOffseting = lhGpuDescriptor;
		lhGpuDescriptorOffseting.Offset(TECHSLOT_SSAO_AO, lSrvSize);
		m_gpu_SRV_Handle1 = lhGpuDescriptorOffseting;
		lhGpuDescriptorOffseting = lhGpuDescriptor;
		lhGpuDescriptorOffseting.Offset(TECHSLOT_SSAO_BLUR, lSrvSize);
		m_gpu_SRV_Handle2 = lhGpuDescriptorOffseting;
	}

	m_rtResources[0]->changeState(m_cmdList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void BlurRender::resize(UINT iwidth, UINT iheight)
{
	RenderBase::resize(iwidth/2, iheight/2);
	m_rtResources[0]->resize(m_width, m_height);

	build_TechDescriptors();
}

void BlurRender::draw(int flags)
{
	m_cmdList->SetGraphicsRootSignature(m_psoLayer.getRootSignature());

	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	// Set DescriptorHeaps  
	ID3D12DescriptorHeap* ldescriptorHeaps[] = { m_descriptorHeap };
	m_cmdList->SetDescriptorHeaps(1, ldescriptorHeaps);

	m_cmdList->SetPipelineState(m_psoLayer.getPSO(OPAQUELAYER));

	// Set RootArguments	
	auto passCB = m_frameResourceManager->getCurrentSSAOCBResource();	
	m_cmdList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress()); // Pass constant buffer data
	m_cmdList->SetGraphicsRootDescriptorTable(3, m_techSRVHandle); // Technical SRV (CubeMap ViewNormal, SSAO maps and etc)
	
	m_cmdList->IASetVertexBuffers(0, 1, &m_mesh->vertexBufferView());
	m_cmdList->IASetIndexBuffer(&m_mesh->indexBufferView());
	m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };

	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_inputResource,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	for (int i = 0; i < m_blurCount; i++)
	{
		int step = i % 2;

		if (step ==0)
		{
			m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_inputResource,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_GENERIC_READ));

			m_rtResources[0]->changeState(m_cmdList,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			
			m_cpu_RTV_Current = m_cpu_RTV_Handle2;
			m_gpu_SRV_Current = m_gpu_SRV_Handle1;
			m_horizontalBlurFlag = 0;
		}
		else
		{
			m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_inputResource,
				D3D12_RESOURCE_STATE_GENERIC_READ, 
				D3D12_RESOURCE_STATE_RENDER_TARGET));

			m_rtResources[0]->changeState(m_cmdList,
				D3D12_RESOURCE_STATE_RENDER_TARGET, 
				D3D12_RESOURCE_STATE_GENERIC_READ);

			m_cpu_RTV_Current = m_cpu_RTV_Handle1;
			m_gpu_SRV_Current = m_gpu_SRV_Handle2;
			m_horizontalBlurFlag = 1;
		}
		
		UINT lTechFlags = 0;
		lTechFlags |= (m_horizontalBlurFlag << 0);
		m_cmdList->SetGraphicsRoot32BitConstant(0, lTechFlags, 0); // Tech Flags		
		m_cmdList->SetGraphicsRootDescriptorTable(1, m_gpu_SRV_Current);
		m_cmdList->OMSetRenderTargets(1, &m_cpu_RTV_Current, true, nullptr);
		m_cmdList->ClearRenderTargetView(m_cpu_RTV_Current, clearColor, 0, nullptr);
		
		// Draw call
		auto drawArg = m_mesh->DrawArgs[m_mesh->Name];
		m_cmdList->DrawIndexedInstanced(drawArg.IndexCount, 1, drawArg.StartIndexLocation, 0, 0);
	}

	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_inputResource,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_GENERIC_READ));
}

void BlurRender::build_screen()
{
	std::vector<VertexGPU> lVertices(4);
	std::vector<uint32_t> lIndices(6);

	VertexGPU vertex = {};

	//v0
	vertex.Pos = XMFLOAT3(-1.0f, -1.0f, 0.0f);
	vertex.Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex.TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
	vertex.UVText = XMFLOAT2(0.0f, 1.0f);
	lVertices[0] = vertex;
	//v1
	vertex.Pos = XMFLOAT3(-1.0f, 1.0f, 0.0f);
	vertex.Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex.UVText = XMFLOAT2(0.0f, 0.0f);
	lVertices[1] = vertex;
	//v2
	vertex.Pos = XMFLOAT3(1.0f, 1.0f, 0.0f);
	vertex.Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex.UVText = XMFLOAT2(1.0f, 0.0f);
	lVertices[2] = vertex;
	//v3
	vertex.Pos = XMFLOAT3(1.0f, -1.0f, 0.0f);
	vertex.Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex.UVText = XMFLOAT2(1.0f, 1.0f);
	lVertices[3] = vertex;

	lIndices[0] = 0;
	lIndices[1] = 1;
	lIndices[2] = 2;

	lIndices[3] = 0;
	lIndices[4] = 2;
	lIndices[5] = 3;

	m_mesh = std::make_unique<Mesh>();
	int vertexByteStride = sizeof(VertexGPU);

	m_mesh->VertexBufferByteSize = vertexByteStride * lVertices.size();
	m_mesh->IndexBufferByteSize = sizeof(uint32_t) * lIndices.size();
	m_mesh->VertexByteStride = vertexByteStride;
	m_mesh->IndexFormat = DXGI_FORMAT_R32_UINT;

	SubMesh submesh = {};
	m_mesh->Name = "screen";
	submesh.IndexCount = lIndices.size();
	m_mesh->DrawArgs[m_mesh->Name] = submesh;

	Utilit3D::UploadMeshToDefaultBuffer<Mesh, VertexGPU, uint32_t>(m_mesh.get(), lVertices, lIndices);
}
