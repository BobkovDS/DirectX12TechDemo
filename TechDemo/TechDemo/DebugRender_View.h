#pragma once
#include "RenderBase.h"
#include "PSODebugLayer_View.h"
class DebugRender_View :
	public RenderBase
{
	IDXGISwapChain3* m_swapChain;
	ComPtr<ID3D12Resource>* m_swapChainResources;

	std::unique_ptr<Mesh> m_mesh;
	PSODebugLayer_View m_psoLayer;	
	
	void buildGeometry();
public:
	DebugRender_View();
	~DebugRender_View();

	void initialize(const RenderMessager& renderParams);
	void build();
	void draw(int flag);	
	void resize(UINT newWidth, UINT newHeight);
	void setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources);
};

