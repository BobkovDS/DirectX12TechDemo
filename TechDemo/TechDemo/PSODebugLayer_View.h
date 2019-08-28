#pragma once
#include "PSOBaseLayer.h"

class PSODebugLayer_View :
	public PSOBaseLayer
{
	void buildShadersBlob();
public:
	PSODebugLayer_View();
	~PSODebugLayer_View();

	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat);
};

