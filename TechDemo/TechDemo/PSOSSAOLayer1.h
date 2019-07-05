#pragma once
#include "PSOBaseLayer.h"
class PSOSSAOLayer1 :
	public PSOBaseLayer
{	
	void buildShadersBlob();

public:
	PSOSSAOLayer1();
	~PSOSSAOLayer1();

	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat);	
};

