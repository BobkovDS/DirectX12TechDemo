#pragma once
#include "PSOBaseLayer.h"
class PSODebugLayer_Screen :
	public PSOBaseLayer
{
	void buildShadersBlob();
public:
	PSODebugLayer_Screen();
	~PSODebugLayer_Screen();

	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat, DXGI_SAMPLE_DESC sampleDesc);
};

