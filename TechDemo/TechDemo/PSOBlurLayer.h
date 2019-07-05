#pragma once
#include "PSOBaseLayer.h"
class PSOBlurLayer :
	public PSOBaseLayer
{
	std::string m_shaderName;
	void buildShadersBlob();
	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat);
public:
	PSOBlurLayer();
	~PSOBlurLayer();

	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat, std::string shaderName);
};

