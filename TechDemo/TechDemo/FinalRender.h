#pragma once
#include "RenderBase.h"
#include "PSOFinalRenderLayer.h"

class FinalRender :
	public RenderBase
{
	IDXGISwapChain3* m_swapChain;
	ComPtr<ID3D12Resource> m_swapChainBuffers[2];
	PSOFinalRenderLayer m_psoLayer;
public:
	FinalRender();
	~FinalRender();

	void initialize(const RenderMessager& renderParams);
	void build();
	void draw(int flag);
};

