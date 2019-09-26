#include "DebugRender_Screen.h"

using namespace DirectX;

DebugRender_Screen::DebugRender_Screen()
{
}

DebugRender_Screen::~DebugRender_Screen()
{
}

void DebugRender_Screen::initialize(const RenderMessager& renderParams)
{	
	RenderBase::initialize(renderParams);
}

void DebugRender_Screen::build()
{
	assert(m_initialized == true);
		
	// Initialize PSO layer
	DXGI_SAMPLE_DESC lSampleDesc;
	lSampleDesc.Count = m_msaaRenderTargets->getSampleCount();
	lSampleDesc.Quality = m_msaaRenderTargets->getSampleQuality();
	m_psoLayer.buildPSO(m_device, m_rtResourceFormat, m_dsResourceFormat, lSampleDesc);

	// Initialize DescriptorHandle: Tech_DescriptorHandle
	m_techSRVHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();

	build_screen();
}

void DebugRender_Screen::draw(UINT textureID)
{
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

	m_cmdList->OMSetRenderTargets(1, &currentRTV, true, nullptr);
	FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };
	m_cmdList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);

	// Set DescriptorHeaps  
	ID3D12DescriptorHeap* ldescriptorHeaps[] = { m_descriptorHeap };
	m_cmdList->SetDescriptorHeaps(1, ldescriptorHeaps);

	m_cmdList->SetPipelineState(m_psoLayer.getPSO(OPAQUELAYER));
	// Set RootArguments		
	m_cmdList->SetGraphicsRootSignature(m_psoLayer.getRootSignature());

	m_cmdList->SetGraphicsRoot32BitConstant(0, textureID, 0); // Tech Flags	
	m_cmdList->SetGraphicsRootDescriptorTable(1, m_techSRVHandle);

	m_cmdList->IASetVertexBuffers(0, 1, &m_mesh->vertexBufferView());
	m_cmdList->IASetIndexBuffer(&m_mesh->indexBufferView());
	m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw call
	auto drawArg = m_mesh->DrawArgs[m_mesh->Name];
	m_cmdList->DrawIndexedInstanced(drawArg.IndexCount, 1, drawArg.StartIndexLocation, 0, 0);

	//-----------------------
	/*m_cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainResources[lResourceIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));
*/
}

void DebugRender_Screen::resize(UINT iwidth, UINT iheight)
{
	RenderBase::resize(iwidth, iheight);
}

void DebugRender_Screen::setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources)
{
	m_swapChainResources = swapChainResources;
}

void DebugRender_Screen::build_screen()
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

	m_mesh->VertexBufferByteSize = vertexByteStride * (UINT) lVertices.size();
	m_mesh->IndexBufferByteSize = sizeof(uint32_t) * (UINT) lIndices.size();
	m_mesh->VertexByteStride = vertexByteStride;
	m_mesh->IndexFormat = DXGI_FORMAT_R32_UINT;

	SubMesh submesh = {};
	m_mesh->Name = "screen";
	submesh.IndexCount = (UINT) lIndices.size();
	m_mesh->DrawArgs[m_mesh->Name] = submesh;

	Utilit3D::UploadMeshToDefaultBuffer<Mesh, VertexGPU, uint32_t>(m_mesh.get(), lVertices, lIndices);
}