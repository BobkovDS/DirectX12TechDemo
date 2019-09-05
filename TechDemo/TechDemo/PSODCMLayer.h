#pragma once
#include "PSOBaseLayer.h"
class PSODCMLayer :
	public PSOBaseLayer
{
	void buildShadersBlob();

public:
	PSODCMLayer();
	~PSODCMLayer();
	
	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat);	
};

