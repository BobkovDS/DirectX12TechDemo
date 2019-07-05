#include "SSAORender.h"
#include <DirectXPackedVector.h>

using namespace DirectX;

SSAORender::SSAORender()
{
}

SSAORender::~SSAORender()
{
}

void SSAORender::initialize(const RenderMessager& renderParams)
{
	RenderBase::initialize(renderParams);
	
	m_viewPortHalf = {};
	m_viewPortHalf.Width = static_cast<float> (renderParams.Width / 2);
	m_viewPortHalf.Height = static_cast<float> (renderParams.Height / 2);
	m_viewPortHalf.MinDepth = 0.0f;
	m_viewPortHalf.MaxDepth = 1.0f;

	m_scissorRectHalf = { 0,0, static_cast<LONG> (renderParams.Width / 2),static_cast<LONG> (renderParams.Height / 2) };

}

void SSAORender::build()
{
	assert(m_initialized == true);

	m_own_resources.resize(3);
	// DepthStencil resources. This class own it.
	{
		create_DescriptorHeap_DSV();
		create_Resource_DS(m_dsResourceFormat);		
		create_DSV();
	}

	// Render Target Resource. This class own it
	{
		create_Resource_RT(m_rtResourceFormat); // for ViewNormal Map
		create_Resource_RT(m_rtResourceFormat, m_width/2, m_height/2); // for AO Map
		create_DescriptorHeap_RTV();
		
		create_RTV(m_rtResourceFormat);
	}

	// Initialize PSO layer
	m_psoLayer1.buildPSO(m_device, m_rtResourceFormat, m_dsResourceFormat);
	m_psoLayer2.buildPSO(m_device, m_rtResourceFormat, m_dsResourceFormat);

	// Initialize both DescriptorHandles: Tech_DescriptorHandle and Texture_DescriptorHandle	
	m_techSRVHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE lhDescriptor(m_techSRVHandle);
		lhDescriptor.Offset(TECHSLOT_RNDVECTORMAP, lSrvSize);
		m_RNDVectorMapHandle = lhDescriptor;
	}
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE lhDescriptor(m_techSRVHandle);
		lhDescriptor.Offset(TECHSRVCOUNT, lSrvSize);
		m_textureSRVHandle = lhDescriptor;
	}
	
	build_randomVectorTexture();
	build_screen();
	build_TechDescriptors();
	m_dsResource->changeState(m_cmdList,D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_rtResources[RESOURCEID_VN]->changeState(m_cmdList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_rtResources[RESOURCEID_AO]->changeState(m_cmdList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);	
}

void SSAORender::build_TechDescriptors()
{
	UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptor(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptorOffseting;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	// SRV for ViewNormal Map
	lhDescriptorOffseting = lhDescriptor;
	lhDescriptorOffseting.Offset(TECHSLOT_SSAO_VSN, lSrvSize);
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_rtResources[RESOURCEID_VN]->getResource()->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;	
	m_device->CreateShaderResourceView(m_rtResources[RESOURCEID_VN]->getResource(), &srvDesc, lhDescriptorOffseting);

	// SRV for Depth Map
	lhDescriptorOffseting = lhDescriptor;
	lhDescriptorOffseting.Offset(TECHSLOT_SSAO_DPT, lSrvSize);
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;// m_dsResource->getResource()->GetDesc().Format;	
	m_device->CreateShaderResourceView(m_dsResource->getResource(), &srvDesc, lhDescriptorOffseting);

	// SRV for AO Map
	lhDescriptorOffseting = lhDescriptor;
	lhDescriptorOffseting.Offset(TECHSLOT_SSAO_AO, lSrvSize);
	srvDesc.Format = m_rtResources[RESOURCEID_AO]->getResource()->GetDesc().Format;	
	m_device->CreateShaderResourceView(m_rtResources[RESOURCEID_AO]->getResource(), &srvDesc, lhDescriptorOffseting);

	// SRV for RandomVector Map
	lhDescriptorOffseting = lhDescriptor;
	lhDescriptorOffseting.Offset(TECHSLOT_RNDVECTORMAP, lSrvSize);
	srvDesc.Format = m_randomVectorsTexture.RessourceInDefaultHeap.Get()->GetDesc().Format;
	m_device->CreateShaderResourceView(m_randomVectorsTexture.RessourceInDefaultHeap.Get(), &srvDesc, lhDescriptorOffseting);
}

void SSAORender::resize(UINT iwidth, UINT iheight)
{
	RenderBase::resize(iwidth, iheight);
	m_dsResource->resize(iwidth, iheight);
	create_DSV();

	m_viewPortHalf= {};
	m_viewPortHalf.Width = static_cast<float> (iwidth/2);
	m_viewPortHalf.Height = static_cast<float> (iheight/2);
	m_viewPortHalf.MinDepth = 0.0f;
	m_viewPortHalf.MaxDepth = 1.0f;

	m_scissorRectHalf = { 0,0, static_cast<LONG> (iwidth/2),static_cast<LONG> (iheight/2) };
}

void SSAORender::draw(int flags)
{	   
	const UINT lcLayerToDraw = 0b010; // SSAO only for simple Opaque objects

	m_cmdList->SetGraphicsRootSignature(m_psoLayer1.getRootSignature());

	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	// Set DescriptorHeaps  
	ID3D12DescriptorHeap* ldescriptorHeaps[] = { m_descriptorHeap };
	m_cmdList->SetDescriptorHeaps(1, ldescriptorHeaps);

	// Set RootArguments
	auto objectCB = m_frameResourceManager->getCurrentObjectCBResource();
	auto passCB = m_frameResourceManager->getCurrentPassCBResource();
	auto boneCB = m_frameResourceManager->getCurrentBoneCBResource();

	UINT lTechFlags = flags;
	m_cmdList->SetGraphicsRoot32BitConstant(0, lTechFlags, 1); // Tech Flags	
	m_cmdList->SetGraphicsRootShaderResourceView(1, objectCB->GetGPUVirtualAddress()); // Instances constant buffer arrray data
	m_cmdList->SetGraphicsRootShaderResourceView(2, m_resourceManager->getMaterialsResource()->GetGPUVirtualAddress());
	m_cmdList->SetGraphicsRootShaderResourceView(3, boneCB->GetGPUVirtualAddress()); // bones constant buffer array data
	m_cmdList->SetGraphicsRootConstantBufferView(4, passCB->GetGPUVirtualAddress()); // Pass constant buffer data
	//m_cmdList->SetGraphicsRootDescriptorTable(5, m_techSRVHandle); // Technical SRV (CubeMap ViewNormal, SSAO maps and etc)
	//m_cmdList->SetGraphicsRootDescriptorTable(6, m_textureSRVHandle); // Textures SRV

	// ---------------- The first Step of SSAO: Drawing ViewNormal and Depth Map
	{
		m_rtResources[RESOURCEID_VN]->changeState(m_cmdList,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_dsResource->changeState(m_cmdList,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);

		CD3DX12_CPU_DESCRIPTOR_HANDLE currentRTV(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
			RESOURCEID_VN, m_rtvDescriptorSize);

		m_cmdList->OMSetRenderTargets(1, &currentRTV, true, &m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

		FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };
		m_cmdList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);
		m_cmdList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		//--- draw calls	
		int lInstanceOffset = 0;
		for (int i = 0; i < m_scene->getLayersCount(); i++) // Draw all Layers
		{
			if (lcLayerToDraw & (1 << i))
			{
				m_cmdList->SetPipelineState(m_psoLayer1.getPSO(i));
				draw_layer(i, lInstanceOffset, true);
			}
		}
	}

	// ---------------- The Second Step of SSAO: Drawing AO map, using ViewNormal and Depth Map
	{
		auto passSSAOCB = m_frameResourceManager->getCurrentSSAOCBResource();

		m_cmdList->SetGraphicsRootSignature(m_psoLayer2.getRootSignature());
		m_cmdList->SetPipelineState(m_psoLayer2.getPSO(OPAQUELAYER));

		m_rtResources[RESOURCEID_VN]->changeState(m_cmdList,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_GENERIC_READ);

		m_rtResources[RESOURCEID_AO]->changeState(m_cmdList,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_dsResource->changeState(m_cmdList,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_GENERIC_READ);
		
		m_cmdList->RSSetViewports(1, &m_viewPortHalf);
		m_cmdList->RSSetScissorRects(1, &m_scissorRectHalf);

		m_cmdList->SetGraphicsRootConstantBufferView(0, passSSAOCB->GetGPUVirtualAddress());
		m_cmdList->SetGraphicsRootDescriptorTable(1, m_RNDVectorMapHandle);

		CD3DX12_CPU_DESCRIPTOR_HANDLE currentRTV(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
			RESOURCEID_AO, m_rtvDescriptorSize);

		m_cmdList->OMSetRenderTargets(1, &currentRTV, true, nullptr);

		FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };
		m_cmdList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);		

		//--- draw calls			

		m_cmdList->IASetVertexBuffers(0, 1, &m_mesh->vertexBufferView());
		m_cmdList->IASetIndexBuffer(&m_mesh->indexBufferView());

		m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto drawArg = m_mesh->DrawArgs[m_mesh->Name];
		m_cmdList->DrawIndexedInstanced(drawArg.IndexCount, 1, drawArg.StartIndexLocation, 0, 0);
		
		// --------------------
		m_rtResources[RESOURCEID_AO]->changeState(m_cmdList,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_GENERIC_READ);
	}
}

void SSAORender::draw_layer(int layerID, int& instanceOffset, bool doDraw)
{
	Scene::SceneLayer* lObjectLayer = m_scene->getLayer(layerID);
	if (lObjectLayer->isLayerVisible()) // Draw Layer if it visible
	{
		for (int ri = 0; ri < lObjectLayer->getSceneObjectCount(); ri++) // One layer has several RenderItems
		{			
			const RenderItem* lMesh = lObjectLayer->getSceneObject(ri)->getObjectMesh();
			int lInstancesCount = lObjectLayer->getSceneObject(ri)->getInstancesCount(); // How much instances for this RenderItem we should draw
			if (lInstancesCount == 0) continue;
			if (doDraw) // some layers we do not need to draw, but we need to count instances for it
			{
				m_cmdList->SetGraphicsRoot32BitConstant(0, instanceOffset, 0); // Instances offset for current layer objects
				auto drawArg = lMesh->Geometry->DrawArgs[lMesh->Geometry->Name];
				m_cmdList->IASetVertexBuffers(0, 1, &lMesh->Geometry->vertexBufferView());
				m_cmdList->IASetIndexBuffer(&lMesh->Geometry->indexBufferView());
				if (layerID != 5)
					m_cmdList->IASetPrimitiveTopology(lMesh->Geometry->PrimitiveType);
				else
					m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

				m_cmdList->DrawIndexedInstanced(drawArg.IndexCount, lInstancesCount, drawArg.StartIndexLocation, 0, 0);
			}
			instanceOffset += lInstancesCount;
		}
	}
}

ID3D12Resource* SSAORender::getVNResource()
{
	return m_rtResources[RESOURCEID_VN]->getResource();
}

ID3D12Resource* SSAORender::getAOResource()
{
	return m_rtResources[RESOURCEID_AO]->getResource();
}

void SSAORender::build_screen()
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

void SSAORender::build_randomVectorTexture()
{
	DirectX::PackedVector::XMCOLOR initData[256 * 256];

	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < 256; j++)
		{
			XMFLOAT3 v(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());
			initData[i * 256 + j] = DirectX::PackedVector::XMCOLOR(v.x, v.y, v.z, 0.0f);
		}
	}

	D3D12_RESOURCE_DESC textDesc = {};
	textDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textDesc.Alignment = 0;
	textDesc.Width = 256;
	textDesc.Height = 256;
	textDesc.DepthOrArraySize = 1;
	textDesc.MipLevels = 1;
	textDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textDesc.SampleDesc.Count = 1;
	textDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT res = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_randomVectorsTexture.RessourceInDefaultHeap));

	assert(SUCCEEDED(res));

	const UINT num2DSubResources = textDesc.DepthOrArraySize * textDesc.MipLevels;
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_randomVectorsTexture.RessourceInDefaultHeap.Get(), 0, num2DSubResources);

	res = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_randomVectorsTexture.RessourceInUploadHeap));

	assert(SUCCEEDED(res));

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = 256 * sizeof(DirectX::PackedVector::XMCOLOR);
	subResourceData.SlicePitch = subResourceData.RowPitch * 256;

	UpdateSubresources(m_cmdList,
		m_randomVectorsTexture.RessourceInDefaultHeap.Get(),
		m_randomVectorsTexture.RessourceInUploadHeap.Get(),
		0, 0,
		num2DSubResources,
		&subResourceData);

	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		m_randomVectorsTexture.RessourceInDefaultHeap.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}