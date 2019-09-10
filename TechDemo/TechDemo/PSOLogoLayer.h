#pragma once
#include "PSOBaseLayer.h"
class PSOLogoLayer :
	public PSOBaseLayer
{
	void buildShadersBlob();
public:
	PSOLogoLayer();
	~PSOLogoLayer();

	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat);
};

