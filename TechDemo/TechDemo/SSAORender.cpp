#include "SSAORender.h"
#include <DirectXPackedVector.h>

using namespace DirectX;

SSAORender::SSAORender():m_AOMapScaleFactor(2)
{
}

SSAORender::~SSAORender()
{
}

void SSAORender::initialize(const RenderMessager& renderParams)
{
	RenderBase::initialize(renderParams);
	
	m_viewPortHalf = {};
	m_viewPortHalf.Width = static_cast<float> (renderParams.Width/ m_AOMapScaleFactor);
	m_viewPortHalf.Height = static_cast<float> (renderParams.Height/ m_AOMapScaleFactor);
	m_viewPortHalf.MinDepth = 0.0f;
	m_viewPortHalf.MaxDepth = 1.0f;

	m_scissorRectHalf = { 0,0, static_cast<LONG> (m_viewPortHalf.Width),static_cast<LONG> (m_viewPortHalf.Height) };
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
		create_Resource_RT(m_viewNormalMapFormat); // for ViewNormal Map		
		create_Resource_RT(AOMapFormat, m_width/ m_AOMapScaleFactor, m_height/ m_AOMapScaleFactor); // for AO Map
		create_DescriptorHeap_RTV(m_rtResources.size());
		
		create_RTV(m_rtResourceFormat);
	}

	// Initialize PSO layer
	DXGI_SAMPLE_DESC lSampleDesc;
	lSampleDesc.Count = 1;
	lSampleDesc.Quality = 0;
	m_psoLayer1.buildPSO(m_device, m_viewNormalMapFormat, m_dsResourceFormat, lSampleDesc);
	m_psoLayer2.buildPSO(m_device, AOMapFormat, m_dsResourceFormat, lSampleDesc);

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

	m_timer.setTickTime(0.0083f);
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
	m_device->CreateShaderResourceView(m_rtResources[RESOURCEID_AO]->getResource(), NULL, lhDescriptorOffseting);

	// SRV for RandomVector Map
	lhDescriptorOffseting = lhDescriptor;
	lhDescriptorOffseting.Offset(TECHSLOT_RNDVECTORMAP, lSrvSize);
	srvDesc.Format = m_randomVectorsTexture.ResourceInDefaultHeap.Get()->GetDesc().Format;
	m_device->CreateShaderResourceView(m_randomVectorsTexture.ResourceInDefaultHeap.Get(), &srvDesc, lhDescriptorOffseting);

	m_dsResource->changeState(m_cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_rtResources[RESOURCEID_VN]->changeState(m_cmdList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_rtResources[RESOURCEID_AO]->changeState(m_cmdList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void SSAORender::resize(UINT iwidth, UINT iheight)
{
	RenderBase::resize(iwidth, iheight);
	m_dsResource->resize(iwidth, iheight);
	m_rtResources[RESOURCEID_VN]->resize(iwidth, iheight);
	m_rtResources[RESOURCEID_AO]->resize(iwidth/ m_AOMapScaleFactor, iheight/ m_AOMapScaleFactor);
	create_DSV(); // Here we create only DepthStencil Descriptor for DS Resource. SRV for it will be created in build_TechDescriptors()
	create_RTV();

	m_viewPortHalf= {};
	m_viewPortHalf.Width = static_cast<float> (iwidth/ m_AOMapScaleFactor);
	m_viewPortHalf.Height = static_cast<float> (iheight/ m_AOMapScaleFactor);
	m_viewPortHalf.MinDepth = 0.0f;
	m_viewPortHalf.MaxDepth = 1.0f;

	m_scissorRectHalf = { 0,0, static_cast<LONG> (iwidth/2),static_cast<LONG> (iheight/ m_AOMapScaleFactor) };

	build_TechDescriptors();
}

void SSAORender::draw(UINT flags)
{		
	const UINT lcLayerWhichMayBeDrawn =
		1 << OPAQUELAYER | 1 << SKINNEDOPAQUELAYER; // SSAO only for Opaque objects

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

	// ---------------- The first Step of SSAO: Drawing ViewNormal and Depth Map
	{	
		{
			const D3D12_RESOURCE_BARRIER lBarries[] = {
					m_rtResources[RESOURCEID_VN]->getBarrier(D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_STATE_RENDER_TARGET),
					m_dsResource->getBarrier(D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE)
			};

			m_cmdList->ResourceBarrier(2, lBarries);
		}

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
			draw_layer(i, lInstanceOffset, lcLayerWhichMayBeDrawn & (1 << i));
		}
	}

	// ---------------- The Second Step of SSAO: Drawing AO map, using ViewNormal and Depth Map
	{
		auto passSSAOCB = m_frameResourceManager->getCurrentSSAOCBResource();

		m_cmdList->SetGraphicsRootSignature(m_psoLayer2.getRootSignature());
		m_cmdList->SetPipelineState(m_psoLayer2.getPSO(OPAQUELAYER));
		
		{
			const D3D12_RESOURCE_BARRIER lBarries[] = {
					m_rtResources[RESOURCEID_VN]->getBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_GENERIC_READ),
					m_rtResources[RESOURCEID_AO]->getBarrier(D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_STATE_RENDER_TARGET),
					m_dsResource->getBarrier(D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ)
			};

			m_cmdList->ResourceBarrier(3, lBarries);
		}

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
	/*
		SSAO Render draw only LOD0 instances for RenderITem. But in our Instances CB we have all intances for all LODs. To count 
		'instanceOffset' correctly, we "draw" all LOD, but do real work only for LOD0, for LOD1..LOD2 we just count InstanceCountByLODLevel
	*/

	Scene::SceneLayer* lObjectLayer = nullptr;
	lObjectLayer = m_scene->getLayer(layerID);
	if (lObjectLayer->isLayerVisible()) // Draw Layer if it visible
	{
		for (int ri = 0; ri < lObjectLayer->getSceneObjectCount(); ri++) // One layer has several RenderItems
		{
			Scene::SceneLayer::SceneLayerObject* lSceneObject = lObjectLayer->getSceneObject(ri);
			int lInstancesCount = lSceneObject->getInstancesCountLOD(); // How much instances for this RenderItem we should draw
			if (lInstancesCount == 0) continue;		

			if (doDraw)
				m_cmdList->SetPipelineState(m_psoLayer1.getPSO(layerID));

			const RenderItem* lRI = lSceneObject->getObjectMesh();

			for (int lod_id = 0; lod_id < LODCOUNT; lod_id++)
			{
				UINT lInstanceCountByLODLevel = lSceneObject->getInstancesCountLOD_byLevel(lod_id);
				if (lInstanceCountByLODLevel == 0) continue;

				bool lLODDraw = (lod_id == 0 | lod_id == 0);// SSAO works only for LOD0 meshes				
				if (doDraw && lLODDraw) // some layers we do not need to draw, but we need to count instances for it
				{
					Mesh* lMesh = lRI->LODGeometry[lod_id];

					if (lMesh == NULL)
					{
						lMesh = lRI->Geometry; // we do not have LOD meshes for this RI
						lod_id = LODCOUNT; // so lets draw it only once
					}

					auto drawArg = lMesh->DrawArgs[lMesh->Name];

					m_cmdList->IASetVertexBuffers(0, 1, &lMesh->vertexBufferView());
					m_cmdList->IASetIndexBuffer(&lMesh->indexBufferView());

					m_cmdList->SetGraphicsRoot32BitConstant(0, instanceOffset, 0); // Instances offset for current layer objects

					if (layerID != NOTOPAQUELAYERGH)
						m_cmdList->IASetPrimitiveTopology(lMesh->PrimitiveType);
					else
						m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);


					m_cmdList->DrawIndexedInstanced(drawArg.IndexCount, lInstanceCountByLODLevel, drawArg.StartIndexLocation, 0, 0);
				}
				instanceOffset += lInstanceCountByLODLevel;				
			}			
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
	vertex.TangentU = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
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

	for (int i = 0; i < 256; ++i)
	{
		for (int j = 0; j < 256; ++j)
		{
			XMFLOAT3 v(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());
			initData[i * 256 + j] = DirectX::PackedVector::XMCOLOR(v.x, v.y, v.z, 0.0f);
		}
	}
	
	m_randomVectorsTexture.ResourceInDefaultHeap = Utilit3D::createTextureWithData(m_device, m_cmdList,
		initData, sizeof(DirectX::PackedVector::XMCOLOR), 256, 256,
		DXGI_FORMAT_R8G8B8A8_UNORM, m_randomVectorsTexture.ResourceInUploadHeap);
}