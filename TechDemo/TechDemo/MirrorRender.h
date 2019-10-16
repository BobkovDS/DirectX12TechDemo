/*
	***************************************************************************************************
	Description:
		Realizes a Mirror reflection for WaterV2 object. It draws selected set of Objects layers/LODs with reflection matrix
		applied to View matrix.
		
		Steps:
			1) FinalRender draws WaterV2 mesh only to Stencil buffer
			2) MirrorRender draws required set of object in reflection, using Stencil buffer
			3) MirrorRender draws WaterV2 mesh using Stencil buffer

	***************************************************************************************************
*/

#pragma once
#include "RenderBase.h"
#include "PSOMirrorLayer.h"
#include "Timer.h"
class MirrorRender :
	public RenderBase
{	
	ComPtr<ID3D12Resource>* m_swapChainResources;
	PSOMirrorLayer m_psoLayer;	
	Timer m_timer;

	void draw_layer(int layerID, int& instanceOffset, bool doDraw);

public:
	MirrorRender();
	~MirrorRender();

	void build();
	void draw(UINT flag);

	void setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources);	
};

