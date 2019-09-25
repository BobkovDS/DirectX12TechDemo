#pragma once
#include "PSOBaseLayer.h"
class PSOComputeLayer :
	public PSOBaseLayer
{
	void buildShadersBlob();

public:
	PSOComputeLayer();
	~PSOComputeLayer();

	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat, DXGI_SAMPLE_DESC sampleDesc);
};

