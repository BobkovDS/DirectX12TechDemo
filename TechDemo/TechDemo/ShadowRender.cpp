#include "ShadowRender.h"

ShadowRender::ShadowRender()
{
}

ShadowRender::~ShadowRender()
{
}


void ShadowRender::initialize(const RenderMessager& renderParams)
{
	RenderBase::initialize(renderParams);
}

void ShadowRender::build()
{
	assert(m_initialized == true);

	m_own_resources.resize(1);
	// DepthStencil resources. This class own it.
	{
		create_DescriptorHeap_DSV();
		create_Resource_DS(m_dsResourceFormat);
		create_DSV();
	}

	// Initialize PSO layer
	m_psoLayer.buildPSO(m_device, m_rtResourceFormat, m_dsResourceFormat);	

	// Initialize both DescriptorHandles: Tech_DescriptorHandle and Texture_DescriptorHandle	
	m_techSRVHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE lhDescriptor(m_techSRVHandle);
		lhDescriptor.Offset(TECHSRVCOUNT, lSrvSize);
		m_textureSRVHandle = lhDescriptor;
	}

	build_TechDescriptors();

	m_timer.setTickTime(0.032633); //60fps;
}

void ShadowRender::build_TechDescriptors()
{
	UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptor(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptorOffseting;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	// SRV for Depth Map
	lhDescriptorOffseting = lhDescriptor;
	lhDescriptorOffseting.Offset(TECHSLOT_SHADOW, lSrvSize);
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;// m_dsResource->getResource()->GetDesc().Format;	
	m_device->CreateShaderResourceView(m_dsResource->getResource(), &srvDesc, lhDescriptorOffseting);
	
	m_dsResource->changeState(m_cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void ShadowRender::resize(UINT iwidth, UINT iheight)
{
	RenderBase::resize(iwidth, iheight); 
	m_dsResource->resize(m_width, m_height);
	create_DSV();

	build_TechDescriptors();
}

void ShadowRender::draw(int lightID)
{
	//if (!m_timer.tick()) return;

	const UINT lcLayerToDraw = 0b1110; // Shadow only for simple Opaque objects

	m_cmdList->SetGraphicsRootSignature(m_psoLayer.getRootSignature());

	m_dsResource->changeState(m_cmdList,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);

	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	// Set DescriptorHeaps  
	ID3D12DescriptorHeap* ldescriptorHeaps[] = { m_descriptorHeap };
	m_cmdList->SetDescriptorHeaps(1, ldescriptorHeaps);

	// Set RootArguments
	auto objectCB = m_frameResourceManager->getCurrentObjectCBResource();
	auto passCB = m_frameResourceManager->getCurrentPassCBResource();
	auto boneCB = m_frameResourceManager->getCurrentBoneCBResource();

	UINT lTechFlags = lightID;
	m_cmdList->SetGraphicsRoot32BitConstant(0, lTechFlags, 1); // Tech Flags	
	m_cmdList->SetGraphicsRootShaderResourceView(1, objectCB->GetGPUVirtualAddress()); // Instances constant buffer arrray data
	m_cmdList->SetGraphicsRootShaderResourceView(2, m_resourceManager->getMaterialsResource()->GetGPUVirtualAddress());
	m_cmdList->SetGraphicsRootShaderResourceView(3, boneCB->GetGPUVirtualAddress()); // bones constant buffer array data
	m_cmdList->SetGraphicsRootConstantBufferView(5, passCB->GetGPUVirtualAddress()); // Pass constant buffer data
	//m_cmdList->SetGraphicsRootDescriptorTable(5, m_techSRVHandle); // Technical SRV (CubeMap ViewNormal, SSAO maps and etc)
	m_cmdList->SetGraphicsRootDescriptorTable(7, m_textureSRVHandle); // Textures SRV
	
	m_cmdList->OMSetRenderTargets(0, nullptr, true, &m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };
	m_cmdList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	{
		//--- draw calls	
		int lInstanceOffset = 0;
		for (int i = 0; i < m_scene->getLayersCount(); i++) // Draw all Layers
		{			
			draw_layer(i, lInstanceOffset, lcLayerToDraw & (1 << i));
		}
	}

	m_dsResource->changeState(m_cmdList,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_GENERIC_READ);
}

void ShadowRender::draw_layer(int layerID, int& instanceOffset, bool doDraw)
{	
	/*
	Shadow Render draw only LOD0 and LOD1 instances for RenderITem. But in our Instances CB we have all intances for all LOD. To count
	'instanceOffset' correctly, we "draw" all LOD, but do real work only for LOD0 and LOD1, for LOD2 we just count InstanceCountByLODLevel
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
				m_cmdList->SetPipelineState(m_psoLayer.getPSO(layerID));

			const RenderItem* lRI = lSceneObject->getObjectMesh();

			for (int lod_id = 0; lod_id < LODCOUNT; lod_id++)
			{
				UINT lInstanceCountByLODLevel = lSceneObject->getInstancesCountLOD_byLevel(lod_id);
				if (lInstanceCountByLODLevel == 0) continue;
								
				bool lLODDraw = (lod_id == 0 || lod_id == 1 ); //Shadow works only for LOD0 and LOD1 meshes				
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
