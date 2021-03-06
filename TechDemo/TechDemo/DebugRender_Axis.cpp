#include "DebugRender_Axis.h"
#include <vector>

DebugRender_Axis::DebugRender_Axis()
{
}

DebugRender_Axis::~DebugRender_Axis()
{
}

void DebugRender_Axis::build()
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
	buildAxisGeometry();
}

void DebugRender_Axis::draw(UINT flags)
{
	int lResourceIndex = m_msaaRenderTargets->getCurrentBufferID();

	CD3DX12_CPU_DESCRIPTOR_HANDLE currentRTV(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		lResourceIndex, m_rtvDescriptorSize);

	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	m_cmdList->OMSetRenderTargets(1, &currentRTV, true, &m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };
	//m_cmdList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);
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
	m_cmdList->IASetVertexBuffers(0, 1, &m_axesMesh->vertexBufferView());
	m_cmdList->IASetIndexBuffer(&m_axesMesh->indexBufferView());

	m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	auto drawArg = m_axesMesh->DrawArgs[m_axesMesh->Name];
	m_cmdList->DrawIndexedInstanced(drawArg.IndexCount, 1, drawArg.StartIndexLocation, 0, 0);	
}

void DebugRender_Axis::resize(UINT iwidth, UINT iheight)
{
	RenderBase::resize(iwidth, iheight);
	m_dsResource->resize(iwidth, iheight);
	create_DSV();
}

void DebugRender_Axis::buildAxisGeometry()
{
	float lLenght = 10.0f;
	float lLabelSize = 0.3f;
	DirectX::XMFLOAT3 lRedColor = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 lGreenColor = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
	DirectX::XMFLOAT3 lBlueColor = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
	std::vector<VertexGPU> lVertices;
	std::vector<uint32_t> lIndices;

	// Axes lines
	VertexGPU px0;
	VertexGPU px1;
	VertexGPU py0;
	VertexGPU py1;
	VertexGPU pz0;
	VertexGPU pz1;

	px0.Pos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	px1.Pos = DirectX::XMFLOAT3(lLenght, 0.0f, 0.0f);
	py0.Pos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	py1.Pos = DirectX::XMFLOAT3(0.0f, lLenght, 0.0f);
	pz0.Pos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	pz1.Pos = DirectX::XMFLOAT3(0.0f, 0.0f, lLenght);

	px0.Normal = lRedColor;
	py0.Normal = lGreenColor;
	pz0.Normal = lBlueColor;
	px1.Normal = px0.Normal;
	py1.Normal = py0.Normal;
	pz1.Normal = pz0.Normal;

	lVertices.push_back(px0);
	lVertices.push_back(px1);
	lVertices.push_back(py0);
	lVertices.push_back(py1);
	lVertices.push_back(pz0);
	lVertices.push_back(pz1);

	lIndices.push_back(0);
	lIndices.push_back(1);
	lIndices.push_back(2);
	lIndices.push_back(3);
	lIndices.push_back(4);
	lIndices.push_back(5);

	// Axes Labels
	// X
	px0.Pos = DirectX::XMFLOAT3(lLenght - lLabelSize, 0.0f, lLabelSize);
	px1.Pos = DirectX::XMFLOAT3(lLenght + lLabelSize, 0.0f, -lLabelSize);
	py0.Pos = DirectX::XMFLOAT3(lLenght + lLabelSize, 0.0f, lLabelSize);
	py1.Pos = DirectX::XMFLOAT3(lLenght - lLabelSize, 0.0f, -lLabelSize);
	py0.Normal = lRedColor;
	py1.Normal = lRedColor;

	lVertices.push_back(px0);
	lVertices.push_back(px1);
	lVertices.push_back(py0);
	lVertices.push_back(py1);
	lIndices.push_back(lIndices.size());
	lIndices.push_back(lIndices.size());
	lIndices.push_back(lIndices.size());
	lIndices.push_back(lIndices.size());

	// Y 
	px0.Pos = DirectX::XMFLOAT3(- lLabelSize, lLenght + lLabelSize*2, 0.0f);
	px1.Pos = DirectX::XMFLOAT3(0, lLenght + lLabelSize , 0.0f);
	py0.Pos = DirectX::XMFLOAT3(lLabelSize, lLenght + lLabelSize * 2, 0.0f);
	py1.Pos = DirectX::XMFLOAT3(-lLabelSize, lLenght, 0.0f);
	px0.Normal = lGreenColor;
	px1.Normal = px0.Normal;	
	py0.Normal = px0.Normal;
	py1.Normal = px0.Normal;

	lVertices.push_back(px0);
	lVertices.push_back(px1);
	lVertices.push_back(py0);
	lVertices.push_back(py1);
	lIndices.push_back(lIndices.size());
	lIndices.push_back(lIndices.size());
	lIndices.push_back(lIndices.size());
	lIndices.push_back(lIndices.size());

	m_axesMesh = std::make_unique<Mesh>();
	SubMesh submesh = {};
	m_axesMesh->Name = "Axes";
	submesh.IndexCount = (UINT) lIndices.size();
	m_axesMesh->DrawArgs[m_axesMesh->Name] = submesh;

	Utilit3D::UploadMeshToDefaultBuffer<Mesh, VertexGPU, uint32_t>(m_axesMesh.get(), lVertices, lIndices);
}

