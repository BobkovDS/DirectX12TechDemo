#include "PSODebugLayer_Normals.h"

using namespace std;

PSODebugLayer_Normals::PSODebugLayer_Normals()
{
}

PSODebugLayer_Normals::~PSODebugLayer_Normals()
{
}

void PSODebugLayer_Normals::buildShadersBlob()
{
	// Compile shaders and store it
	string basedir = "Shaders\\";
	string shaderName = "DebugNormalsRender_shaders.hlsl";	
	//shaderName = "DebugLightRender_shaders.hlsl";	
	string fullFileName = basedir + shaderName;	

	string shaderType = "none";
	try
	{
		shaderType = "vs";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "VS", "vs_5_1");		

		shaderType = "gs_vn";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "GS_VN", "gs_5_1");

		shaderType = "gs_fn";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "GS_FN", "gs_5_1");

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

void PSODebugLayer_Normals::buildPSO(ID3D12Device* device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat)
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
	psoDescLayer0.GS = { reinterpret_cast<BYTE*>(m_shaders["gs_vn"]->GetBufferPointer()), m_shaders["gs_vn"]->GetBufferSize() };
	psoDescLayer0.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

	// PSO for Layer_1:  Non-skinned Not Opaque objects: [NOTOPAQUELAYER]
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer1 = psoDescLayer0;
	psoDescLayer1.GS = { reinterpret_cast<BYTE*>(m_shaders["gs_fn"]->GetBufferPointer()), m_shaders["gs_fn"]->GetBufferSize() };
		
	// Create PSO objects
	// Vertex Normals PSO
	HRESULT res = m_device->CreateGraphicsPipelineState(&psoDescLayer0, IID_PPV_ARGS(&m_pso[DRN_VERTEX]));
	if (res != S_OK)
	{
		_com_error err(res);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" ");
		throw MyCommonRuntimeException(errMsg, L"PSO creation");
	}

	// Face Normals PSO
	res = m_device->CreateGraphicsPipelineState(&psoDescLayer1, IID_PPV_ARGS(&m_pso[DRN_FACE]));
	if (res != S_OK)
	{
		_com_error err(res);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" ");
		throw MyCommonRuntimeException(errMsg, L"PSO creation");
	}	
}