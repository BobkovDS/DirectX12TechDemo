#include "MirrorRender.h"

MirrorRender::MirrorRender()
{
}

MirrorRender::~MirrorRender()
{
}

void MirrorRender::initialize(const RenderMessager& renderParams)
{	
	RenderBase::initialize(renderParams);
}

void MirrorRender::build()
{
	assert(m_initialized == true);

	// Initialize PSO layer
	DXGI_SAMPLE_DESC lSampleDesc;
	lSampleDesc.Count = m_msaaRenderTargets->getSampleCount();
	lSampleDesc.Quality = m_msaaRenderTargets->getSampleQuality();
	m_psoLayer.buildPSO(m_device, m_rtResourceFormat, m_dsResourceFormat, lSampleDesc);

	// Initialize both DescriptorHandles: Tech_DescriptorHandle and Texture_DescriptorHandle	
	m_techSRVHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_GPU_DESCRIPTOR_HANDLE lhDescriptor(m_techSRVHandle);
	lhDescriptor.Offset(TECHSRVCOUNT, lSrvSize);
	m_textureSRVHandle = lhDescriptor;	
}

void MirrorRender::setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources)
{
	m_swapChainResources = swapChainResources;
}

void MirrorRender::resize(UINT iwidth, UINT iheight)
{
	RenderBase::resize(iwidth, iheight);	
}

void MirrorRender::draw(int flags)
{
	Scene::SceneLayer* lObjectLayer = nullptr;
	lObjectLayer = m_scene->getLayer(NOTOPAQUELAYERCH);

	for (int ri = 0; ri < 1; ri++) // Now we think that we have only one WaterV2 object
	{
		Scene::SceneLayer::SceneLayerObject* lSceneObject = lObjectLayer->getSceneObject(ri);
		if (lSceneObject == nullptr) return;
		int lInstancesCount = lSceneObject->getInstancesCountLOD(); // How much instances for this RenderItem we should draw
		if (lInstancesCount == 0) return; // If WaterV2 object is out or ViewFrustum, no need to draw other Reflections
	}

	const UINT lcLayerWhichMayBeDrawn =
		 ((1 << OPAQUELAYER) | (1 << NOTOPAQUELAYER) | (1 << SKINNEDOPAQUELAYER) | (1 << NOTOPAQUELAYERCH));
		

	int lResourceIndex = m_msaaRenderTargets->getCurrentBufferID();

	//m_cmdList->ResourceBarrier(1,
	//	&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainResources[lResourceIndex].Get(),
	//		D3D12_RESOURCE_STATE_PRESENT,
	//		D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE currentRTV(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		lResourceIndex, m_rtvDescriptorSize);

	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	m_cmdList->OMSetRenderTargets(1, &currentRTV, true, &m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };		
	//m_cmdList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);
	m_cmdList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//m_cmdList->OMSetStencilRef(0);
	m_cmdList->SetGraphicsRootSignature(m_psoLayer.getRootSignature());

	// Set DescriptorHeaps  
	ID3D12DescriptorHeap* ldescriptorHeaps[] = { m_descriptorHeap };
	m_cmdList->SetDescriptorHeaps(1, ldescriptorHeaps);

	// Set RootArguments
	auto objectCB = m_frameResourceManager->getCurrentObjectCBResource();
	auto passCB = m_frameResourceManager->getCurrentPassCBResource();
	auto boneCB = m_frameResourceManager->getCurrentBoneCBResource();	

	UINT passAdressOffset = m_frameResourceManager->getPassCBsize();
	D3D12_GPU_VIRTUAL_ADDRESS lPassCBBaseAdress = passCB->GetGPUVirtualAddress();		

	UINT lTechFlags = 0;
	m_cmdList->SetGraphicsRoot32BitConstant(0, lTechFlags, 1); // Tech Flags	
	m_cmdList->SetGraphicsRootShaderResourceView(1, objectCB->GetGPUVirtualAddress()); // Instances constant buffer arrray data
	m_cmdList->SetGraphicsRootShaderResourceView(2, m_resourceManager->getMaterialsResource()->GetGPUVirtualAddress());
	m_cmdList->SetGraphicsRootShaderResourceView(3, boneCB->GetGPUVirtualAddress()); // bones constant buffer array data	
	m_cmdList->SetGraphicsRootDescriptorTable(6, m_techSRVHandle); // Technical SRV (CubeMap ViewNormal, SSAO maps and etc)
	m_cmdList->SetGraphicsRootDescriptorTable(7, m_textureSRVHandle); // Textures SRV

	//--- draw calls	
	
	// 0. A Mirror stencil information was drawn befor in FinalRender.

	// 1. Draw a scene in reflection way 
	int lInstanceOffset = 0;
	int lLayerToDraw = 0;
	m_cmdList->OMSetStencilRef(1);

	if (flags) // Mirror reflection drawing is turn-off
	{	
		D3D12_GPU_VIRTUAL_ADDRESS lReflectionPassCB = lPassCBBaseAdress + passAdressOffset; // Skip the first PassCB - Main PassCB, to get Reflection Pass CB	
		m_cmdList->SetGraphicsRootConstantBufferView(5, lReflectionPassCB); // Pass constant buffer data		
		lLayerToDraw = lcLayerWhichMayBeDrawn & ~(1 << NOTOPAQUELAYERCH); // All Layers, which can be drawn here, except a WaterV2 layer
		for (int i = 0; i < m_scene->getLayersCount(); i++) // Draw all Layers		
			draw_layer(i, lInstanceOffset, lLayerToDraw & (1 << i));
	}

	// 2. Draw a mirror 
	m_cmdList->SetGraphicsRootConstantBufferView(5, lPassCBBaseAdress); // Pass constant buffer data
	lInstanceOffset = 0;
	lLayerToDraw = lcLayerWhichMayBeDrawn & (1 << NOTOPAQUELAYERCH); // Only WaterV2 layer (it uses WaterV2 object for stenceling)
	for (int i = 0; i < m_scene->getLayersCount(); i++) // Draw all Layers			
		draw_layer(i, lInstanceOffset, lLayerToDraw & (1 << i));

	////-----------------------
	//m_cmdList->ResourceBarrier(1,
	//	&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainResources[lResourceIndex].Get(),
	//		D3D12_RESOURCE_STATE_RENDER_TARGET,
	//		D3D12_RESOURCE_STATE_PRESENT));
}


void MirrorRender::draw_layer(int layerID, int& instanceOffset, bool doDraw)
{
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
				
				bool lLODDraw = ((lod_id == 0) | (lod_id == 0));// In Mirror Reflection we draw only Instances with level: LOD0 and LOD1...

				if (lRI->ExcludeFromReflection)
					lLODDraw = false;

				if (doDraw && lLODDraw) // some layers we do not need to draw, but we need to count instances for it
				{
					Mesh* lMesh = lRI->LODGeometry[lod_id +1]; // ...but for drawing we use LOD1 and LOD2 mesh 

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