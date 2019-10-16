#include "DebugRender_Light.h"
#include <vector>

DebugRender_Light::DebugRender_Light()
{
}

DebugRender_Light::~DebugRender_Light()
{
}

void DebugRender_Light::build()
{
	assert(m_initialized == true);

	m_doesRenderUseMutliSampling = true;

	// DepthStencil resources. This class own it.
	{
		create_DescriptorHeap_DSV();
		create_Resource_DS(m_dsResourceFormat);
		create_DSV();
	}

	// Initialize PSO layer
	DXGI_SAMPLE_DESC lSampleDesc;
	lSampleDesc.Count = m_msaaRenderTargets->getSampleCount();
	lSampleDesc.Quality = m_msaaRenderTargets->getSampleQuality();
	m_psoLayer.buildPSO(m_device, m_rtResourceFormat, m_dsResourceFormat, lSampleDesc);

	// Build Axis geometry
	buildGeometry();
}

void DebugRender_Light::draw(UINT flags)
{
	int lResourceIndex = m_msaaRenderTargets->getCurrentBufferID();	

	CD3DX12_CPU_DESCRIPTOR_HANDLE currentRTV(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		lResourceIndex, m_rtvDescriptorSize);

	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	m_cmdList->OMSetRenderTargets(1, &currentRTV, true, &m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };	
	m_cmdList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_cmdList->SetGraphicsRootSignature(m_psoLayer.getRootSignature());

	// Set RootArguments
	auto objectCB = m_frameResourceManager->getCurrentObjectCBResource();
	auto passCB = m_frameResourceManager->getCurrentPassCBResource();

	UINT lTechFlags = flags;
	m_cmdList->SetGraphicsRoot32BitConstant(0, lTechFlags, 1); // Tech Flags
	m_cmdList->SetGraphicsRootShaderResourceView(1, objectCB->GetGPUVirtualAddress()); // Instances constant buffer arrray data
	m_cmdList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress()); // Pass constant buffer data

	m_cmdList->SetPipelineState(m_psoLayer.getPSO(OPAQUELAYER));
	m_cmdList->SetGraphicsRoot32BitConstant(0, 0, 0); // Instances offset for current layer objects

	//--- draw calls	
	m_cmdList->IASetVertexBuffers(0, 1, &m_mesh->vertexBufferView());
	m_cmdList->IASetIndexBuffer(&m_mesh->indexBufferView());

	m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	auto drawArg = m_mesh->DrawArgs[m_mesh->Name];
	UINT lInstancesCount = (UINT) m_scene->getLights().size();
	m_cmdList->DrawIndexedInstanced(drawArg.IndexCount, lInstancesCount, drawArg.StartIndexLocation, 0, 0);
}

void DebugRender_Light::resize(UINT iwidth, UINT iheight)
{
	RenderBase::resize(iwidth, iheight);
	m_dsResource->resize(iwidth, iheight);
	create_DSV();
}

void DebugRender_Light::buildGeometry()
{
	std::vector<VertexGPU> lVertices;
	std::vector<uint32_t> lIndices;

	VertexGPU lDummyVertex = {};	

	lVertices.push_back(lDummyVertex);	
	lIndices.push_back(0);	

	m_mesh = std::make_unique<Mesh>();
	SubMesh submesh = {};
	m_mesh->Name = "Light";
	submesh.IndexCount = (UINT) lIndices.size();
	m_mesh->DrawArgs[m_mesh->Name] = submesh;

	Utilit3D::UploadMeshToDefaultBuffer<Mesh, VertexGPU, uint32_t>(m_mesh.get(), lVertices, lIndices);
}

