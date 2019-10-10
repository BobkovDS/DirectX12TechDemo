#include "PSOSSAOLayer3.h"

using namespace std;

PSOSSAOLayer3::PSOSSAOLayer3()
{
}

PSOSSAOLayer3::~PSOSSAOLayer3()
{
}

void PSOSSAOLayer3::buildShadersBlob()
{
	// Compile shaders and store it
	string basedir = "Shaders\\";
	string shaderName = "SSAORender_shaders3.hlsl";
	string fullFileName = basedir + shaderName;

	string shaderType = "none";
	try
	{
		shaderType = "vs";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "VS", "vs_5_1");

		shaderType = "ps";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "PS", "ps_5_1");

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

void PSOSSAOLayer3::buildPSO(ID3D12Device* device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat, DXGI_SAMPLE_DESC sampleDesc)
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
	psoDescLayer0.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDescLayer0.DepthStencilState.DepthEnable = false;
	psoDescLayer0.SampleDesc = sampleDesc;

	// Create PSO objects	
	HRESULT res = m_device->CreateGraphicsPipelineState(&psoDescLayer0, IID_PPV_ARGS(&m_pso[OPAQUELAYER]));
	if (res != S_OK)
	{
		_com_error err(res);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" ");
		throw MyCommonRuntimeException(errMsg, L"PSO creation");
	}
}
