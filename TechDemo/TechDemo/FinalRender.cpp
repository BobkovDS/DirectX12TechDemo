#include "FinalRender.h"

FinalRender::FinalRender():m_swapChain(nullptr)
{
}

FinalRender::~FinalRender()
{
}

void FinalRender::initialize(const RenderMessager& renderParams)
{	
	m_swapChain = renderParams.SwapChain;

	RenderBase::initialize(renderParams);	
}

void FinalRender::build()
{
	assert(m_initialized == true);

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
		UINT lSrvSize =m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_GPU_DESCRIPTOR_HANDLE lhDescriptor(m_techSRVHandle);
		lhDescriptor.Offset(TECHSRVCOUNT, lSrvSize); 	
	m_textureSRVHandle = lhDescriptor;
}


void FinalRender::setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources)
{
	m_swapChainResources = swapChainResources;
}

void FinalRender::resize(UINT iwidth, UINT iheight)
{
	RenderBase::resize(iwidth, iheight);	
	m_dsResource->resize(iwidth, iheight);
	create_DSV();
}

void FinalRender::draw(int flags)
{	
	int lResourceIndex = m_swapChain->GetCurrentBackBufferIndex();
	m_cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainResources[lResourceIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE currentRTV(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		lResourceIndex,	m_rtvDescriptorSize);

	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	m_cmdList->OMSetRenderTargets(1, &currentRTV, true, &m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };
	m_cmdList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);
	m_cmdList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_cmdList->OMSetStencilRef(0);

	m_cmdList->SetGraphicsRootSignature(m_psoLayer.getRootSignature());

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
	m_cmdList->SetGraphicsRootDescriptorTable(5, m_techSRVHandle); // Technical SRV (ViewNormal, SSAO maps and etc)
	m_cmdList->SetGraphicsRootDescriptorTable(6, m_textureSRVHandle); // Textures SRV
	
	//--- draw calls	
	int lInstanceOffset = 0;
	for (int i = 0; i < m_scene->getLayersCount(); i++) // Draw all Layers
	{
		Scene::SceneLayer* lObjectLayer = m_scene->getLayer(i);
		if (lObjectLayer->isLayerVisible()) // Draw Layer if it visible
		{
			m_cmdList->SetPipelineState(m_psoLayer.getPSO(i)); // Here we change shaders. As we have the one RootSignauture for Render, so Root areguments are not reset when we set new PSO

			//int lInstanceOffset = m_scene->getLayerInstanceOffset(i); // How many Instances were on prev layers

			for (int ri = 0; ri < lObjectLayer->getSceneObjectCount(); ri++) // One layer has several RenderItems
			{
				m_cmdList->SetGraphicsRoot32BitConstant(0, lInstanceOffset, 0); // Instances offset for current layer objects
				const RenderItem* lMesh = lObjectLayer->getSceneObject(ri)->getObjectMesh();
				int lInstancesCount = lObjectLayer->getSceneObject(ri)->getInstancesCount(); // How much instances for this RenderItem we should draw
				auto drawArg = lMesh->Geometry->DrawArgs[lMesh->Geometry->Name];
								
				m_cmdList->IASetVertexBuffers(0, 1, &lMesh->Geometry->vertexBufferView());
				m_cmdList->IASetIndexBuffer(&lMesh->Geometry->indexBufferView());
				m_cmdList->IASetPrimitiveTopology(lMesh->Geometry->PrimitiveType);
				m_cmdList->DrawIndexedInstanced(drawArg.IndexCount, lInstancesCount, drawArg.StartIndexLocation, 0, 0);
				lInstanceOffset += lInstancesCount;
			}
		}
	}

	//-----------------------
	m_cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainResources[lResourceIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));
}