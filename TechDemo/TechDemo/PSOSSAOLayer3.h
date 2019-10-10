#pragma once
#include "PSOBaseLayer.h"
class PSOSSAOLayer3 :
	public PSOBaseLayer
{
	void buildShadersBlob();

public:
	PSOSSAOLayer3();
	~PSOSSAOLayer3();

	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat, DXGI_SAMPLE_DESC sampleDesc);
};

