#pragma once
#include "RenderBase.h"
#include "PSODebugLayer_Screen.h"

class DebugRender_Screen :
	public RenderBase
{
	IDXGISwapChain3* m_swapChain;
	ComPtr<ID3D12Resource>* m_swapChainResources;

	PSODebugLayer_Screen m_psoLayer;
	std::unique_ptr<Mesh> m_mesh;
	void build_screen();
public:
	DebugRender_Screen();
	~DebugRender_Screen();

	void initialize(const RenderMessager& renderParams);
	void build();
	void draw(UINT textureID);
	void resize(UINT newWidth, UINT newHeight);	
	void setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources);
};

