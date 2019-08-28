#include "DebugRender_Normals.h"
#include <vector>

DebugRender_Normals::DebugRender_Normals()
{
}

DebugRender_Normals::~DebugRender_Normals()
{
}

void DebugRender_Normals::initialize(const RenderMessager& renderParams)
{
	m_swapChain = renderParams.SwapChain;
	RenderBase::initialize(renderParams);
}

void DebugRender_Normals::build()
{
	assert(m_initialized == true);

	//This class does not own it DepthStencil resources	

	// Initialize PSO layer
	m_psoLayer.buildPSO(m_device, m_rtResourceFormat, m_dsResourceFormat);

	// Build Axis geometry
	buildGeometry();
}

void DebugRender_Normals::draw(int flags)
{
	int lResourceIndex = m_swapChain->GetCurrentBackBufferIndex();
	m_cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainResources[lResourceIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE currentRTV(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		lResourceIndex, m_rtvDescriptorSize);

	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	m_cmdList->OMSetRenderTargets(1, &currentRTV, true, &m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	//FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };
	////m_cmdList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);
	//m_cmdList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
	//	D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_cmdList->SetGraphicsRootSignature(m_psoLayer.getRootSignature());

	// Set RootArguments
	auto objectCB = m_frameResourceManager->getCurrentObjectCBResource();
	auto passCB = m_frameResourceManager->getCurrentPassCBResource();

	UINT lTechFlags = flags;
	m_cmdList->SetGraphicsRoot32BitConstant(0, lTechFlags, 1); // Tech Flags	
	m_cmdList->SetGraphicsRootShaderResourceView(1, objectCB->GetGPUVirtualAddress()); // Instances constant buffer arrray data		
	m_cmdList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress()); // Pass constant buffer data

	//--- draw calls	
	UINT lLayerToDraw = 0b110; //OPAQUELAYER and NOTOPAQUELAYER
	//if (flags & (1 << DRN_VERTEX)) // Which Normals (Vertex, Face or Tangent) we can draw
	{
		int lInstanceOffset = 0;
		m_cmdList->SetPipelineState(m_psoLayer.getPSO(DRN_VERTEX)); // Here we change shaders. As we have the one RootSignauture for Render, so Root areguments are not reset when we set new PSO	
		for (int i = 0; i < m_scene->getLayersCount(); i++)
			draw_layer(i, lInstanceOffset, (lLayerToDraw & (1<<i)), D3D_PRIMITIVE_TOPOLOGY_POINTLIST);		
	}

	//-----------------------
	m_cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainResources[lResourceIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));
}

void DebugRender_Normals::draw_layer(int layerID, int& instanseOffset, bool doDraw, D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology)
{	
	Scene::SceneLayer* lObjectLayer = nullptr;
	lObjectLayer = m_scene->getLayer(layerID);

	if (lObjectLayer->isLayerVisible()) // Draw Layer if it visible
	{
		for (int ri = 0; ri < lObjectLayer->getSceneObjectCount(); ri++) // One layer has several RenderItems
		{
			m_cmdList->SetGraphicsRoot32BitConstant(0, instanseOffset, 0); // Instances offset for current layer objects
			const RenderItem* lMesh = lObjectLayer->getSceneObject(ri)->getObjectMesh();
			int lInstancesCount = lObjectLayer->getSceneObject(ri)->getInstancesCount(); // How much instances for this RenderItem we should draw
			if (lInstancesCount == 0) return;
			if (doDraw) // sometimes we need not to Draw but count a number of instances it use
			{
				auto drawArg = lMesh->Geometry->DrawArgs[lMesh->Geometry->Name];
				m_cmdList->IASetVertexBuffers(0, 1, &lMesh->Geometry->vertexBufferView());
				m_cmdList->IASetIndexBuffer(&lMesh->Geometry->indexBufferView());
				m_cmdList->IASetPrimitiveTopology(PrimitiveTopology);
				m_cmdList->DrawIndexedInstanced(drawArg.IndexCount, lInstancesCount, drawArg.StartIndexLocation, 0, 0);
			}
			instanseOffset += lInstancesCount;
		}
	}
}

void DebugRender_Normals::setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources)
{
	m_swapChainResources = swapChainResources;
}

void DebugRender_Normals::resize(UINT iwidth, UINT iheight)
{
	RenderBase::resize(iwidth, iheight);
	
	if (!m_dsvHeapWasSetBefore) // if we not use "our" DS. For DebugRender_Normals we should.
	{
		create_DSV();	
		m_dsResource->resize(iwidth, iheight);
	}
}

void DebugRender_Normals::buildGeometry()
{
}

