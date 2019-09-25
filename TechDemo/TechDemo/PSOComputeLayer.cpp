#include "PSOComputeLayer.h"

using namespace std;

PSOComputeLayer::PSOComputeLayer()
{
}

PSOComputeLayer::~PSOComputeLayer()
{
}

void PSOComputeLayer::buildShadersBlob()
{
	// Compile shaders and store it
	string basedir = "Shaders\\";
	string fullFileName = basedir + "ComputeRender_shaders.hlsl";

	string shaderType = "none";
	try
	{
		shaderType = "cs";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "CS", "cs_5_1");
	}
	catch (MyCommonRuntimeException& e)
	{
		throw MyCommonRuntimeException(
			e.what() + L" for (" + wstring(shaderType.begin(), shaderType.end()) + L")",
			e.where()
		);
	}

	// Create RootSignature.
	buildRootSignature(m_shaders["cs"].Get()); //Root signature was added to Vertex Shader only. It is enough for us.
}

void PSOComputeLayer::buildPSO(ID3D12Device* device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat, DXGI_SAMPLE_DESC sampleDesc)
{
	m_device = device;
	m_rtvFormat = rtFormat;
	m_dsvFormat = dsFormat;

	// compile shaders blob
	buildShadersBlob();

	// PSO for Layer_0: Non-skinned Opaque objects: [OPAQUELAYER]
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDescLayer0 = {};

	psoDescLayer0.pRootSignature = m_rootSignature.Get();
	psoDescLayer0.CS = { reinterpret_cast<BYTE*>(m_shaders["cs"]->GetBufferPointer()), m_shaders["cs"]->GetBufferSize() };

	// Create PSO objects
	//OPAQUELAYER
	HRESULT res = m_device->CreateComputePipelineState(&psoDescLayer0, IID_PPV_ARGS(&m_pso[OPAQUELAYER]));
	if (res != S_OK)
	{
		_com_error err(res);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" ");
		throw MyCommonRuntimeException(errMsg, L"PSO creation");
	}
}