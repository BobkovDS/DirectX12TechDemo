#include "DCMRender.h"


DCMRender::DCMRender() 
{
}

DCMRender::~DCMRender()
{
}

using namespace DirectX;


void DCMRender::initialize(const RenderMessager& renderParams)
{
	RenderBase::initialize(renderParams);	
}

void DCMRender::build()
{
	assert(m_initialized == true);

	// Create Resources (6 Textures and 7 Views for it: 1 Cube SRV and 6 RTV; + DS Resource and DSV for it)

	m_own_resources.resize(2); // 1 for DS and 1 for RT resources

	// DepthStencil resources. This class own it.
	{
		create_DescriptorHeap_DSV();
		create_Resource_DS(m_dsResourceFormat);
		create_DSV();
	}

	// Render Target Resources. Dynamic Cube Map resources
	{
		create_Resource_RT(m_rtResourceFormat, m_width, m_height, 6);
		create_DescriptorHeap_RTV(6);
		create_RTV(m_rtResourceFormat);
		m_rtResources[0]->changeState(m_cmdList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	// Initialize both DescriptorHandles: Tech_DescriptorHandle and Texture_DescriptorHandle	
	m_techSRVHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_GPU_DESCRIPTOR_HANDLE lhDescriptor(m_techSRVHandle);
	lhDescriptor.Offset(TECHSRVCOUNT, lSrvSize);
	m_textureSRVHandle = lhDescriptor;

	build_TechDescriptors();

	// Initialize PSO layer
	m_psoLayer.buildPSO(m_device, m_rtResourceFormat, m_dsResourceFormat);	

	m_timer.setTickTime(0.0083f);	
}

void DCMRender::build_TechDescriptors()
{
	// Initialize both DescriptorHandles: Tech_DescriptorHandle and Texture_DescriptorHandle	
	UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	// 1. Create Cube SRV for DynamicCube map resources
	{		
		CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptor(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart());
		lhDescriptor.Offset(TECHSLOT_DCM, lSrvSize);		

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = m_rtResourceFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = 1;

		m_device->CreateShaderResourceView(m_rtResources[0]->getResource(), &srvDesc, lhDescriptor);
	}
}


void DCMRender::create_RTV(DXGI_FORMAT viewFormat)
{
	DXGI_FORMAT lViewFormat = viewFormat;
	if (lViewFormat == DXGI_FORMAT_UNKNOWN) lViewFormat = m_rtResourceFormat;
	
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = lViewFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < 6; i++)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		m_device->CreateRenderTargetView(m_rtResources[0]->getResource(), &rtvDesc, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
	}	
}

void DCMRender::resize(UINT iwidth, UINT iheight)
{
	
	RenderBase::resize(iwidth, iheight);
	m_dsResource->resize(iwidth, iheight);
	m_rtResources[0]->resize(iwidth, iheight);
	
	create_DSV(); 
	create_RTV();

	build_TechDescriptors();
}

void DCMRender::draw(int flags)
{
	if (!m_timer.tick()) return;

	const UINT lcLayerToDraw = 0b1011; // SSAO only for Opaque objects //NOTE: If it is required to add new layer, PSO should be for this created in PSODCMLayer

	m_cmdList->SetGraphicsRootSignature(m_psoLayer.getRootSignature());

	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	// Set DescriptorHeaps  
	ID3D12DescriptorHeap* ldescriptorHeaps[] = { m_descriptorHeap };
	m_cmdList->SetDescriptorHeaps(1, ldescriptorHeaps);

	// Set RootArguments
	auto objectCB = m_frameResourceManager->getCurrentObjectCBResource();
	auto passCB = m_frameResourceManager->getCurrentPassCBResource();
	auto boneCB = m_frameResourceManager->getCurrentBoneCBResource();
	auto drawCB = m_frameResourceManager->getCurrentDrawIDCBResource();

	UINT lTechFlags = flags;
	m_cmdList->SetGraphicsRoot32BitConstant(0, lTechFlags, 1); // Tech Flags	
	m_cmdList->SetGraphicsRootShaderResourceView(1, objectCB->GetGPUVirtualAddress()); // Instances constant buffer arrray data
	m_cmdList->SetGraphicsRootShaderResourceView(2, m_resourceManager->getMaterialsResource()->GetGPUVirtualAddress());
	m_cmdList->SetGraphicsRootShaderResourceView(3, boneCB->GetGPUVirtualAddress()); // bones constant buffer array data
	m_cmdList->SetGraphicsRootShaderResourceView(4, drawCB->GetGPUVirtualAddress()); // DrawInstancesID constant buffer array data	
	m_cmdList->SetGraphicsRootDescriptorTable(6, m_techSRVHandle); // Technical SRV (CubeMap ViewNormal, SSAO maps and etc)
	m_cmdList->SetGraphicsRootDescriptorTable(7, m_textureSRVHandle); // Textures SRV

	m_rtResources[0]->changeState(m_cmdList, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

	UINT passAdressOffset = m_frameResourceManager->getPassCBsize();
	D3D12_GPU_VIRTUAL_ADDRESS lPassCBBaseAdress = passCB->GetGPUVirtualAddress();
	lPassCBBaseAdress += passAdressOffset; // Skip the first PassCB

	for (int side_id = 0; side_id < 6; side_id++)
	{		
		m_cmdList->SetGraphicsRootConstantBufferView(5, lPassCBBaseAdress); // Pass constant buffer data
		CD3DX12_CPU_DESCRIPTOR_HANDLE currentRTV(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
			side_id, m_rtvDescriptorSize);

		m_cmdList->OMSetRenderTargets(1, &currentRTV, true, &m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

		FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };
		m_cmdList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);
		m_cmdList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		//	//--- draw calls	
		int lInstanceOffset = 0;
		for (int i = 0; i < m_scene->getLayersCount(); i++) // Draw all Layers
		{
			draw_layer(i, lInstanceOffset, lcLayerToDraw & (1 << i));
		}

		lPassCBBaseAdress += passAdressOffset;
	}

	m_rtResources[0]->changeState(m_cmdList,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_GENERIC_READ);
}

void DCMRender::draw_layer(int layerID, int& instanceOffset, bool doDraw)
{
	/*
		SSAO Render draw only LOD0 instances for RenderITem. But in our Instances CB we have all intances for all LOD. To count
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
				m_cmdList->SetPipelineState(m_psoLayer.getPSO(layerID));

			const RenderItem* lRI = lSceneObject->getObjectMesh();

			for (int lod_id = 0; lod_id < LODCOUNT; lod_id++)
			{
				UINT lInstanceCountByLODLevel = lSceneObject->getInstancesCountLOD_byLevel(lod_id);
				if (lInstanceCountByLODLevel == 0) continue;

				bool lLODDraw = (lod_id == 0);// SSAO works only for LOD0 meshes				
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