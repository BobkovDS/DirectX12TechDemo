#pragma once
#include "PSOBaseLayer.h"
class PSODebugLayer_Axis :
	public PSOBaseLayer
{		
	void buildShadersBlob();
public:
	PSODebugLayer_Axis();
	~PSODebugLayer_Axis();

	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat);
};

