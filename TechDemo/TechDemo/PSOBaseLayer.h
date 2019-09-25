#pragma once
#include "stdafx.h"
#include <map>
#include "Defines.h"
#include "Utilit3D.h"
#include "ApplException.h"

class PSOBaseLayer
{
protected:
	static std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout[ILVCOUNT];
	std::map<std::string, Microsoft::WRL::ComPtr <ID3DBlob>> m_shaders;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso[LAYERSCOUNT];
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	ID3D12Device* m_device;
	DXGI_FORMAT m_rtvFormat;
	DXGI_FORMAT m_dsvFormat;

	virtual void buildShadersBlob() = 0;
	void buildRootSignature(ID3DBlob* ptrblob);
	D3D12_GRAPHICS_PIPELINE_STATE_DESC buildCommonPSODescription(DXGI_SAMPLE_DESC sampleDesc);
public:
	PSOBaseLayer();
	virtual ~PSOBaseLayer();

	//virtual void initialized()=0;
	virtual void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat, DXGI_SAMPLE_DESC sampleDesc) = 0;
	ID3D12PipelineState* getPSO(UINT layerID);
	ID3D12RootSignature* getRootSignature();	
};

