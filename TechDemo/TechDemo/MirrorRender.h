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

	void initialize(const RenderMessager& renderParams);
	void build();
	void draw(int flag);
	void resize(UINT newWidth, UINT newHeight);
	void setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources);	
};

