#include "FinalRender.h"

FinalRender::FinalRender():m_swapChain(nullptr)
{
}

FinalRender::~FinalRender()
{
}

void FinalRender::initialize(const RenderMessager& renderParams)
{
	m_swapChain = dynamic_cast<IDXGISwapChain3*>(renderParams.SwapChain);

	RenderBase::initialize(renderParams);
}

void FinalRender::build()
{
	assert(m_initialized == true);

	// DepthStencil resources. This class own it.
	{
		create_Resource_DS(m_dsResourceFormat);
		create_DescriptorHeap_DSV();
		create_DSV();
	}

	//RenderTarget resources. This class do not own no one RTV resources (neither RT Resources, nor rtvHeap)
	{
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_swapChainBuffers[0]));
		m_swapChain->GetBuffer(1, IID_PPV_ARGS(&m_swapChainBuffers[2]));

		ID3D12Resource* lSwapChainResources[2] = { m_swapChainBuffers[0].Get(), m_swapChainBuffers[1].Get() };
		
		set_Resource_RT(m_swapChainBuffers[0].Get());
		set_Resource_RT(m_swapChainBuffers[1].Get());			
	}
}

void FinalRender::draw(int flags)
{	
	int lResourceIndex = m_swapChain->GetCurrentBackBufferIndex();
	m_cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_rtResources[lResourceIndex],
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
	m_cmdList->SetDescriptorHeaps(m_descriptorHeaps.size(), m_descriptorHeaps.data());
	

	// Set RootArguments
	auto objectCB = m_frameResourceManager->getCurrentObjectCBResource();
	auto passCB = m_frameResourceManager->getCurrentObjectCBResource();
	auto boneCB = m_frameResourceManager->getCurrentObjectCBResource();

	UINT lTechFlags = 0;//Test
	m_cmdList->SetGraphicsRoot32BitConstant(0, lTechFlags, 1); // Tech Flags
	m_cmdList->SetGraphicsRootShaderResourceView(1, objectCB->GetGPUVirtualAddress()); // Instances constant buffer arrray data
	//m_cmdList->SetGraphicsRootShaderResourceView(2, m_scene.getMaterialsResource()->GetGPUVirtualAddress());
	m_cmdList->SetGraphicsRootShaderResourceView(3, boneCB->GetGPUVirtualAddress()); // bones constant buffer array data
	m_cmdList->SetGraphicsRootConstantBufferView(4, passCB->GetGPUVirtualAddress()); // Pass constant buffer data
	m_cmdList->SetGraphicsRootDescriptorTable(5, m_descriptorHeaps[0]->GetGPUDescriptorHandleForHeapStart()); // Technical SRV (ViewNormal, SSAO maps and etc)
	m_cmdList->SetGraphicsRootDescriptorTable(6, m_descriptorHeaps[1]->GetGPUDescriptorHandleForHeapStart()); // Textures SRV
	
	//--- draw calls	
	for (int i = 0; i < m_scene->getLayersCount(); i++) // Draw all Layers
	{
		Scene::SceneLayer* lObjectLayer = m_scene->getLayer(i);
		if (lObjectLayer->isLayerVisible()) // Draw Layer if it visible
		{
			m_cmdList->SetPipelineState(m_psoLayer.getPSO(i)); // Here we change shaders. As we have the one RootSignauture for Render, so Root areguments are not reset when we set new PSO

			int lInstanceOffset = m_scene->getLayerInstanceOffset(i); // How many Instances were on prev layers
			m_cmdList->SetGraphicsRoot32BitConstant(0, lInstanceOffset, 0); // Instances offset for current layer objects

			for (int ri = 0; ri < lObjectLayer->getSceneObjectCount(); ri++) // One layer has several RenderItems
			{
				const RenderItem* lMesh = lObjectLayer->getSceneObject(ri)->getObjectMesh();
				int lInstancesCount = lObjectLayer->getSceneObject(ri)->getInstances().size(); // How much instances for this RenderItem we should draw
				auto drawArg = lMesh->Geometry->DrawArgs[lMesh->Geometry->Name];

				m_cmdList->IASetVertexBuffers(0, 1, &lMesh->Geometry->vertexBufferView());
				m_cmdList->IASetIndexBuffer(&lMesh->Geometry->indexBufferView());

				m_cmdList->DrawIndexedInstanced(drawArg.IndexCount, lInstancesCount, drawArg.StartIndexLocation, 0, 0);
			}
		}
	}

	//-----------------------
	m_cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_rtResources[lResourceIndex],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));
}