#pragma once
#include "PSOBaseLayer.h"

class PSODebugLayer_Normals :
	public PSOBaseLayer
{
	void buildShadersBlob();
public:
	PSODebugLayer_Normals();
	~PSODebugLayer_Normals();

	void buildPSO(ID3D12Device* m_device, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat);
};

