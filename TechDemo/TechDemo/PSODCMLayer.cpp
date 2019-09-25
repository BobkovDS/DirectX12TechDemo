#include "PSODCMLayer.h"

using namespace std;

PSODCMLayer::PSODCMLayer()
{
}

PSODCMLayer::~PSODCMLayer()
{
}


void PSODCMLayer::buildShadersBlob()
{
	// Compile shaders and store it
	string basedir = "Shaders\\";
	string fullFileName = basedir + "DCMRender_shaders.hlsl";
	string fullFileName_skinned = basedir + "DCMRender_shaders_skinned.hlsl";
	string fullFileName_sky = basedir + "DCMRender_Sky_shaders.hlsl";

	string shaderType = "none";
	try
	{
		shaderType = "vs";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "VS", "vs_5_1");

		shaderType = "vs";
		m_shaders["vs_skinned"] = Utilit3D::compileShader(fullFileName_skinned, NULL, "VS", "vs_5_1");
		
		shaderType = "vs_sky";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName_sky, NULL, "VS", "vs_5_1");

		shaderType = "ps";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "PS", "ps_5_1");

		shaderType = "ps_skinned";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "PS", "ps_5_1");

		shaderType = "ps_sky";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName_sky, NULL, "PS", "ps_5_1");
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

void PSODCMLayer::buildPSO(ID3D12Device* device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat, DXGI_SAMPLE_DESC sampleDesc)
{
	m_device = device;
	m_rtvFormat = rtFormat;
	m_dsvFormat = dsFormat;

	// compile shaders blob
	buildShadersBlob();

	// PSO for Layer_0: Non-skinned Opaque objects: [OPAQUELAYER]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer0 = buildCommonPSODescription(sampleDesc);
	psoDescLayer0.InputLayout = { m_inputLayout[ILV1].data(), (UINT)m_inputLayout[ILV1].size() };
	psoDescLayer0.VS = { reinterpret_cast<BYTE*>(m_shaders["vs"]->GetBufferPointer()), m_shaders["vs"]->GetBufferSize() };
	psoDescLayer0.PS = { reinterpret_cast<BYTE*>(m_shaders["ps"]->GetBufferPointer()), m_shaders["ps"]->GetBufferSize() };

	if (m_shaders.find("gs") != m_shaders.end())
		psoDescLayer0.GS = { reinterpret_cast<BYTE*>(m_shaders["gs"]->GetBufferPointer()), m_shaders["gs"]->GetBufferSize() };

	// PSO for Layer_3: Skinned Not Opaque objects: [SKINNEDNOTOPAQUELAYER]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer1 = buildCommonPSODescription(sampleDesc);
	psoDescLayer1.InputLayout = { m_inputLayout[ILV2].data(), (UINT)m_inputLayout[ILV2].size() };
	psoDescLayer1.VS = { reinterpret_cast<BYTE*>(m_shaders["vs_skinned"]->GetBufferPointer()), m_shaders["vs_skinned"]->GetBufferSize() };
	psoDescLayer1.PS = { reinterpret_cast<BYTE*>(m_shaders["ps_skinned"]->GetBufferPointer()), m_shaders["ps_skinned"]->GetBufferSize() };

	// PSO for Layer_5: The Sky Cube map object: [SKY]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer5 = buildCommonPSODescription(sampleDesc);
	psoDescLayer5.InputLayout = { m_inputLayout[ILV1].data(), (UINT)m_inputLayout[ILV1].size() };
	psoDescLayer5.VS = { reinterpret_cast<BYTE*>(m_shaders["vs_sky"]->GetBufferPointer()), m_shaders["vs_sky"]->GetBufferSize() };
	psoDescLayer5.PS = { reinterpret_cast<BYTE*>(m_shaders["ps_sky"]->GetBufferPointer()), m_shaders["ps_sky"]->GetBufferSize() };
	psoDescLayer5.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL; // =1 - Far side
	psoDescLayer5.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	

	// Create PSO objects
	//OPAQUELAYER
	HRESULT res = m_device->CreateGraphicsPipelineState(&psoDescLayer0, IID_PPV_ARGS(&m_pso[OPAQUELAYER]));
	if (res != S_OK)
	{
		_com_error err(res);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" ");
		throw MyCommonRuntimeException(errMsg, L"PSO creation");
	}

	//SKINNEDOPAQUELAYER
	res = m_device->CreateGraphicsPipelineState(&psoDescLayer1, IID_PPV_ARGS(&m_pso[SKINNEDOPAQUELAYER]));
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
}