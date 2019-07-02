#pragma once
#include "RenderBase.h"
#include "PSODebugLayer_Light.h"
class DebugRender_Light :
	public RenderBase
{
	IDXGISwapChain3* m_swapChain;
	ComPtr<ID3D12Resource>* m_swapChainResources;

	std::unique_ptr<Mesh> m_mesh;
	PSODebugLayer_Light m_psoLayer;

	void buildGeometry();
public:
	DebugRender_Light();
	~DebugRender_Light();

	void initialize(const RenderMessager& renderParams);
	void build();
	void draw(int flag);
	void resize(UINT newWidth, UINT newHeight);
	void setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources);
};

