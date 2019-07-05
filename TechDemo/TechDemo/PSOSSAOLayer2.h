#pragma once
#include "PSOBaseLayer.h"
class PSOSSAOLayer2 :
	public PSOBaseLayer
{	
	void buildShadersBlob();

public:
	PSOSSAOLayer2();
	~PSOSSAOLayer2();

	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat);
};

