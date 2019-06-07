#include "PSOBaseLayer.h"


std::vector<D3D12_INPUT_ELEMENT_DESC> PSOBaseLayer::m_inputLayout[] =
{
	// NON-SKINNED variant
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		
	},
	// SKINNED variant
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },		
		{ "BONEINDEX", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDEX", 1, DXGI_FORMAT_R32G32B32A32_UINT, 0, 64, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDEX", 2, DXGI_FORMAT_R32G32B32A32_UINT, 0, 80, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDEX", 3, DXGI_FORMAT_R32G32B32A32_UINT, 0, 96, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 112, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEWEIGHT", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 128, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEWEIGHT", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 144, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEWEIGHT", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 160, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	}
};


PSOBaseLayer::PSOBaseLayer()
{
}

PSOBaseLayer::~PSOBaseLayer()
{
}

ID3D12PipelineState* PSOBaseLayer::getPSO(UINT layerID)
{
	if (layerID < LAYERSCOUNT)
		return m_pso[layerID].Get();
	else
		return nullptr;
}

ID3D12RootSignature* PSOBaseLayer::getRootSignature()
{
	return m_rootSignature.Get();
}

void PSOBaseLayer::buildRootSignature(ID3DBlob* ptrblob)
{
	HRESULT res = m_device->CreateRootSignature(
		0,
		ptrblob->GetBufferPointer(),
		ptrblob->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature));

	if (!SUCCEEDED(res))
	{
		_com_error err(res);
		std::wstring errMsg = err.ErrorMessage() + std::wstring(L" ");
		throw MyCommonRuntimeException(errMsg, L"RootSignature creation");
	}
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC PSOBaseLayer::buildCommonPSODescription()
{
	/* return common PSO description:		
		- Stencil test is turn off
		- Blending is turn off
		- FillMode is D3D12_FILL_MODE_SOLID
		- FrontCounterClockwise is True;
	*/
	assert(m_shaders["vs"]); //did we call buildShadersBlob() before?

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		
	psoDesc.pRootSignature = m_rootSignature.Get();

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	// Define which winding is for Front-face: CounterClockwise or Clockwise
	psoDesc.RasterizerState.FrontCounterClockwise = false; // Vertex winding order which we got from Obj(?) is Clockwise,	
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // we have it only for our Flashing Red
	//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK; // Do not draw Back-faces. For performances
	psoDesc.RasterizerState.DepthBias = 100000;
	psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	psoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	psoDesc.NumRenderTargets = 1;	
	psoDesc.SampleDesc.Count = 1;

	return psoDesc;
}