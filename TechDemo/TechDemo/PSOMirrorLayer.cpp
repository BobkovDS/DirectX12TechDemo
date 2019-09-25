#include "PSOMirrorLayer.h"

using namespace std;

PSOMirrorLayer::PSOMirrorLayer()
{
}

PSOMirrorLayer::~PSOMirrorLayer()
{
}

void PSOMirrorLayer::buildShadersBlob()
{
	// Compile shaders and store it
	string basedir = "Shaders\\";
	string shaderName = "MirrorRender_shaders.hlsl";		
	string shaderNameSky = "MirrorRender_Sky_shaders.hlsl";
	string skinedShaderName = "MirrorRender_skinnedShaders.hlsl";
	string shaderNameCH = "MirrorRender_CH_shaders.hlsl";

	string fullFileName = basedir + shaderName;	
	string fullFileNameCH = basedir + shaderNameCH;
	string fullFileNameSky = basedir + shaderNameSky;
	string skinnedFullFileName = basedir + skinedShaderName;
	
	D3D_SHADER_MACRO lShadersDefinitions[] = { "MIRBLEND", "0.6f" , NULL, NULL };

	string shaderType = "none";
	try
	{
		shaderType = "vs";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "VS", "vs_5_1");		

		shaderType = "vs_cs";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileNameCH, NULL, "VS", "vs_5_1");

		shaderType = "vs_sky";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileNameSky, NULL, "VS", "vs_5_1");

		shaderType = "vs_skinned";
		m_shaders[shaderType] = Utilit3D::compileShader(skinnedFullFileName, NULL, "VS", "vs_5_1");

		shaderType = "ps";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, lShadersDefinitions, "PS", "ps_5_1");

		shaderType = "ps_cs";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileNameCH, NULL, "PS", "ps_5_1");

		shaderType = "ps_sky";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileNameSky, NULL, "PS", "ps_5_1");

		shaderType = "ps_skinned";
		m_shaders[shaderType] = Utilit3D::compileShader(skinnedFullFileName, lShadersDefinitions, "PS", "ps_5_1");
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

void PSOMirrorLayer::buildPSO(ID3D12Device* device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat, DXGI_SAMPLE_DESC sampleDesc)
{
	m_device = device;
	m_rtvFormat = rtFormat;
	m_dsvFormat = dsFormat;

	// compile shaders blob
	buildShadersBlob();

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

	// PSO for Layer_0: Non-skinned Opaque objects: [OPAQUELAYER]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer0 = buildCommonPSODescription(sampleDesc);
	psoDescLayer0.InputLayout = { m_inputLayout[ILV1].data(), (UINT)m_inputLayout[ILV1].size() };
	psoDescLayer0.VS = { reinterpret_cast<BYTE*>(m_shaders["vs"]->GetBufferPointer()), m_shaders["vs"]->GetBufferSize() };
	psoDescLayer0.PS = { reinterpret_cast<BYTE*>(m_shaders["ps"]->GetBufferPointer()), m_shaders["ps"]->GetBufferSize() };	
	psoDescLayer0.BlendState = CD3DX12_BLEND_DESC(blend_desc);
	psoDescLayer0.DepthStencilState.StencilEnable = true; // Turn Stenciling on
	psoDescLayer0.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL; // to pass it should be = 1		
	//psoDescLayer0.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR; // 

	psoDescLayer0.RasterizerState.FrontCounterClockwise = true;

	if (m_shaders.find("gs") != m_shaders.end())
		psoDescLayer0.GS = { reinterpret_cast<BYTE*>(m_shaders["gs"]->GetBufferPointer()), m_shaders["gs"]->GetBufferSize() };

	// PSO for Layer_1:  Non-skinned Not Opaque objects: [NOTOPAQUELAYER]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer1 = psoDescLayer0;	
	psoDescLayer1.BlendState = CD3DX12_BLEND_DESC(blend_desc);
	psoDescLayer1.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDescLayer1.DepthStencilState.BackFace.StencilFunc = psoDescLayer1.DepthStencilState.FrontFace.StencilFunc;

	// PSO for Layer_2: Skinned Opaque objects: [SKINNEDOPAQUELAYER]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer2 = buildCommonPSODescription(sampleDesc);
	psoDescLayer2.InputLayout = { m_inputLayout[ILV2].data(),(UINT)m_inputLayout[ILV2].size() };
	psoDescLayer2.VS = { reinterpret_cast<BYTE*>(m_shaders["vs_skinned"]->GetBufferPointer()), m_shaders["vs_skinned"]->GetBufferSize() };
	psoDescLayer2.PS = { reinterpret_cast<BYTE*>(m_shaders["ps_skinned"]->GetBufferPointer()), m_shaders["ps_skinned"]->GetBufferSize() };
	psoDescLayer2.BlendState = CD3DX12_BLEND_DESC(blend_desc);
	psoDescLayer2.DepthStencilState.StencilEnable = true; // Turn Stenciling on
	psoDescLayer2.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL; // to pass it should be = 1	
	//psoDescLayer2.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;  
	psoDescLayer2.RasterizerState.FrontCounterClockwise = true;

	// PSO for Layer_3: Skinned Not Opaque objects: [SKINNEDNOTOPAQUELAYER]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer3 = psoDescLayer1;
	psoDescLayer3.InputLayout = { m_inputLayout[ILV2].data(),(UINT)m_inputLayout[ILV2].size() };
	psoDescLayer3.VS = { reinterpret_cast<BYTE*>(m_shaders["vs_skinned"]->GetBufferPointer()), m_shaders["vs_skinned"]->GetBufferSize() };
	psoDescLayer3.PS = { reinterpret_cast<BYTE*>(m_shaders["ps_skinned"]->GetBufferPointer()), m_shaders["ps_skinned"]->GetBufferSize() };
	psoDescLayer3.GS = psoDescLayer2.GS;	

		// PSO for Layer_5: The Sky Cube map object: [SKY]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer5 = buildCommonPSODescription(sampleDesc);
	psoDescLayer5.InputLayout = { m_inputLayout[ILV1].data(), (UINT)m_inputLayout[ILV1].size() };
	psoDescLayer5.VS = { reinterpret_cast<BYTE*>(m_shaders["vs_sky"]->GetBufferPointer()), m_shaders["vs_sky"]->GetBufferSize() };
	psoDescLayer5.PS = { reinterpret_cast<BYTE*>(m_shaders["ps_sky"]->GetBufferPointer()), m_shaders["ps_sky"]->GetBufferSize() };
	psoDescLayer5.BlendState = CD3DX12_BLEND_DESC(blend_desc);
	psoDescLayer5.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL; // =1 - Far side
	psoDescLayer5.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDescLayer5.DepthStencilState.StencilEnable = true; // Turn Stenciling on
	psoDescLayer5.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL; // to pass it should be = 1	
	psoDescLayer5.RasterizerState.FrontCounterClockwise = true;	

	// PSO for Layer_6: Not Opaque with Compute Shader objects: [NOTOPAQUELAYERCH]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer6 = buildCommonPSODescription(sampleDesc);
	psoDescLayer6.InputLayout = { m_inputLayout[ILV1].data(), (UINT)m_inputLayout[ILV1].size() };
	psoDescLayer6.VS = { reinterpret_cast<BYTE*>(m_shaders["vs_cs"]->GetBufferPointer()), m_shaders["vs_cs"]->GetBufferSize() };
	psoDescLayer6.PS = { reinterpret_cast<BYTE*>(m_shaders["ps_cs"]->GetBufferPointer()), m_shaders["ps_cs"]->GetBufferSize() };
	psoDescLayer6.BlendState = CD3DX12_BLEND_DESC(blend_desc);
	psoDescLayer6.DepthStencilState.StencilEnable = true; // Turn Stenciling on	
	psoDescLayer6.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;; // to pass it should be = 1		

	//psoDescLayer6.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

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

	//SKY
	res = m_device->CreateGraphicsPipelineState(&psoDescLayer5, IID_PPV_ARGS(&m_pso[SKY]));
	if (res != S_OK)
	{
		_com_error err(res);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" ");
		throw MyCommonRuntimeException(errMsg, L"PSO creation");
	}

	//NOTOPAQUELAYERCH
	res = m_device->CreateGraphicsPipelineState(&psoDescLayer6, IID_PPV_ARGS(&m_pso[NOTOPAQUELAYERCH]));
	if (res != S_OK)
	{
		_com_error err(res);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" ");
		throw MyCommonRuntimeException(errMsg, L"PSO creation");
	}
}