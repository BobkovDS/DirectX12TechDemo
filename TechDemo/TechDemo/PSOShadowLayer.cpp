#include "PSOShadowLayer.h"

using namespace std;

PSOShadowLayer::PSOShadowLayer()
{
}

PSOShadowLayer::~PSOShadowLayer()
{
}

void PSOShadowLayer::buildShadersBlob()
{
	// Compile shaders and store it
	string basedir = "Shaders\\";
	string fullFileName = basedir + "ShadowRender_shaders.hlsl";
	string fullFileName_skinned = basedir + "ShadowRender_shaders_skinned.hlsl";
	D3D_SHADER_MACRO lShader_Macros[] = { "ALPHA_TEST", "1", NULL, NULL };

	string shaderType = "none";
	try
	{
		shaderType = "vs";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "VS", "vs_5_1");
		
		shaderType = "vs_skinned";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName_skinned, NULL, "VS", "vs_5_1");

		shaderType = "ps";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "PS", "ps_5_1");

		shaderType = "ps_at"; //at - Aplha Test
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, lShader_Macros, "PS", "ps_5_1");

		shaderType = "ps_skinned";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName_skinned, NULL, "PS", "ps_5_1");
	}
	catch (MyCommonRuntimeException& e)
	{
		throw MyCommonRuntimeException(
			e.what() + L" for (" + wstring(shaderType.begin(), shaderType.end()) + L")",
			e.where()
		);
	}

	// Create RootSignature.
	buildRootSignature(m_shaders["vs"].Get()); //Root signature was added to Vertex Shader only. It is enough for us.
}

void PSOShadowLayer::buildPSO(ID3D12Device* device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat)
{
	m_device = device;
	m_rtvFormat = rtFormat;
	m_dsvFormat = dsFormat;

	// compile shaders blob
	buildShadersBlob();

	// PSO for Layer_0: Non-skinned Opaque objects: [OPAQUELAYER]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer0 = buildCommonPSODescription();
	psoDescLayer0.InputLayout = { m_inputLayout[ILV1].data(), (UINT)m_inputLayout[ILV1].size() };
	psoDescLayer0.VS = { reinterpret_cast<BYTE*>(m_shaders["vs"]->GetBufferPointer()), m_shaders["vs"]->GetBufferSize() };
	psoDescLayer0.PS = { reinterpret_cast<BYTE*>(m_shaders["ps"]->GetBufferPointer()), m_shaders["ps"]->GetBufferSize() };
	psoDescLayer0.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	psoDescLayer0.NumRenderTargets = 0;
	// PSO for Layer_1:  Non-skinned Not Opaque objects: [NOTOPAQUELAYER]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer1 = psoDescLayer0;
	D3D12_BLEND_DESC  blend_desc = {};
	blend_desc.RenderTarget[0].BlendEnable = true;
	blend_desc.RenderTarget[0].LogicOpEnable = false;
	blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blend_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blend_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDescLayer1.BlendState = CD3DX12_BLEND_DESC(blend_desc);
	psoDescLayer1.PS = { reinterpret_cast<BYTE*>(m_shaders["ps_at"]->GetBufferPointer()), m_shaders["ps_at"]->GetBufferSize() };

	// PSO for Layer_2: Skinned Opaque objects: [SKINNEDOPAQUELAYER]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer2 = buildCommonPSODescription();
	psoDescLayer2.InputLayout = { m_inputLayout[ILV2].data(),(UINT)m_inputLayout[ILV2].size() };
	psoDescLayer2.VS = { reinterpret_cast<BYTE*>(m_shaders["vs_skinned"]->GetBufferPointer()), m_shaders["vs_skinned"]->GetBufferSize() };
	psoDescLayer2.PS = { reinterpret_cast<BYTE*>(m_shaders["ps_skinned"]->GetBufferPointer()), m_shaders["ps_skinned"]->GetBufferSize() };
	psoDescLayer2.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	psoDescLayer2.NumRenderTargets = 0;

	// PSO for Layer_3: Skinned Not Opaque objects: [SKINNEDNOTOPAQUELAYER]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer3 = psoDescLayer1;
	psoDescLayer3.InputLayout = { m_inputLayout[ILV2].data(),(UINT)m_inputLayout[ILV2].size() };
	psoDescLayer3.VS = { reinterpret_cast<BYTE*>(m_shaders["vs_skinned"]->GetBufferPointer()), m_shaders["vs_skinned"]->GetBufferSize() };
	psoDescLayer3.PS = { reinterpret_cast<BYTE*>(m_shaders["ps_skinned"]->GetBufferPointer()), m_shaders["ps_skinned"]->GetBufferSize() };
	psoDescLayer3.GS = psoDescLayer2.GS;

	// Create PSO objects
	//OPAQUELAYER
	HRESULT res = m_device->CreateGraphicsPipelineState(&psoDescLayer0, IID_PPV_ARGS(&m_pso[OPAQUELAYER]));
	if (res != S_OK)
	{
		_com_error err(res);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" ");
		throw MyCommonRuntimeException(errMsg, L"PSO creation");
	}

	//NOTOPAQUELAYER
	res = m_device->CreateGraphicsPipelineState(&psoDescLayer1, IID_PPV_ARGS(&m_pso[NOTOPAQUELAYER]));
	if (res != S_OK)
	{
		_com_error err(res);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" ");
		throw MyCommonRuntimeException(errMsg, L"PSO creation");
	}

	//SKINNEDOPAQUELAYER
	res = m_device->CreateGraphicsPipelineState(&psoDescLayer2, IID_PPV_ARGS(&m_pso[SKINNEDOPAQUELAYER]));
	if (res != S_OK)
	{
		_com_error err(res);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" ");
		throw MyCommonRuntimeException(errMsg, L"PSO creation");
	}

	//SKINNEDNOTOPAQUELAYER
	res = m_device->CreateGraphicsPipelineState(&psoDescLayer3, IID_PPV_ARGS(&m_pso[SKINNEDNOTOPAQUELAYER]));
	if (res != S_OK)
	{
		_com_error err(res);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" ");
		throw MyCommonRuntimeException(errMsg, L"PSO creation");
	}
}