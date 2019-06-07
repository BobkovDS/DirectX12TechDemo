#include "PSOFinalRenderLayer.h"

using namespace std;

PSOFinalRenderLayer::PSOFinalRenderLayer()
{		
}

PSOFinalRenderLayer::~PSOFinalRenderLayer()
{
}

void PSOFinalRenderLayer::buildShadersBlob()
{
	// Compile shaders and store it
	string basedir = "Shaders\\";
	string shaderName = "FinalRender_shaders.hlsl";
	string skinedShaderName = "FinalRender_skinnedShaders.hlsl";
	string fullFileName = basedir + shaderName;
	string skinnedFullFileName = basedir + skinedShaderName;

	string shaderType = "none";
	try
	{
		shaderType = "vs";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "VS", "vs_5_1");

		shaderType = "vs_skinned";
		m_shaders[shaderType] = Utilit3D::compileShader(skinnedFullFileName, NULL, "VS", "vs_5_1");

		shaderType = "ps";
		m_shaders[shaderType] = Utilit3D::compileShader(fullFileName, NULL, "PS", "ps_5_1");

		shaderType = "ps_skinned";
		m_shaders[shaderType] = Utilit3D::compileShader(skinnedFullFileName, NULL, "PS", "ps_5_1");

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

void PSOFinalRenderLayer::buildPSO(ID3D12Device* device)
{
	m_device = device;

	// compile shaders blob
	buildShadersBlob();

	// PSO for Layer_0: Non-skinned Opaque objects
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer0 = buildCommonPSODescription();
	psoDescLayer0.InputLayout = { m_inputLayout[ILV1].data(), (UINT) m_inputLayout[ILV1].size() };
	psoDescLayer0.VS = { reinterpret_cast<BYTE*>(m_shaders["vs"]->GetBufferPointer()), m_shaders["vs"]->GetBufferSize() };
	psoDescLayer0.PS = { reinterpret_cast<BYTE*>(m_shaders["ps"]->GetBufferPointer()), m_shaders["ps"]->GetBufferSize() };
	
	// PSO for Layer_1:  Non-skinned Not Opaque objects
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer1 = psoDescLayer0;
	D3D12_BLEND_DESC  blend_desc = {};
	blend_desc.RenderTarget[0].BlendEnable = true;
	blend_desc.RenderTarget[0].LogicOpEnable = false;
	blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	blend_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	blend_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDescLayer1.BlendState = CD3DX12_BLEND_DESC(blend_desc);

	// PSO for Layer_2: Skinned Opaque objects
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer2 = buildCommonPSODescription();
	psoDescLayer2.InputLayout = { m_inputLayout[ILV2].data(),(UINT)m_inputLayout[ILV2].size() };
	psoDescLayer2.VS = { reinterpret_cast<BYTE*>(m_shaders["vs_skinned"]->GetBufferPointer()), m_shaders["vs_skinned"]->GetBufferSize() };
	psoDescLayer2.PS = { reinterpret_cast<BYTE*>(m_shaders["ps_skinned"]->GetBufferPointer()), m_shaders["ps_skinned"]->GetBufferSize() };

	// PSO for Layer_3: Skinned Not Opaque objects
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLayer3 = psoDescLayer1;
	psoDescLayer3.InputLayout = { m_inputLayout[ILV2].data(),(UINT)m_inputLayout[ILV2].size() };
	psoDescLayer3.VS = { reinterpret_cast<BYTE*>(m_shaders["vs_skinned"]->GetBufferPointer()), m_shaders["vs_skinned"]->GetBufferSize() };
	psoDescLayer3.PS = { reinterpret_cast<BYTE*>(m_shaders["ps_skinned"]->GetBufferPointer()), m_shaders["ps_skinned"]->GetBufferSize() };
}