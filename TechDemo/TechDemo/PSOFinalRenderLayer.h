#pragma once
#include "PSOBaseLayer.h"

class PSOFinalRenderLayer :
	public PSOBaseLayer
{
	void buildShadersBlob();
public:
	PSOFinalRenderLayer();
	~PSOFinalRenderLayer();

	//void initialized();
	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat);
};

