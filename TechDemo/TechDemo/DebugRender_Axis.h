#pragma once
#include "RenderBase.h"
#include "PSODebugLayer_Axis.h"

class DebugRender_Axis :
	public RenderBase
{
	IDXGISwapChain3* m_swapChain;
	ComPtr<ID3D12Resource>* m_swapChainResources;
	
	std::unique_ptr<Mesh> m_axesMesh;
	PSODebugLayer_Axis m_psoLayer;

	void buildAxisGeometry();

public:
	DebugRender_Axis();
	~DebugRender_Axis();

	void initialize(const RenderMessager& renderParams);
	void build();
	void draw(int flag);
	void resize(UINT newWidth, UINT newHeight);
	void setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources);
};

