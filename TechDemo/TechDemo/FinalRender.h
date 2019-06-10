#pragma once
#include "RenderBase.h"
#include "PSOFinalRenderLayer.h"
/*
		FINAL RENDER.
		This render does final render for a scene, using results of any other Renders like SSAO, ShadowMap.
		It does in normal way - just take all geometry, draws it with required shaders, using two packages of texture: 
		Tech_textures(Other Renders results) and Textures. A key moment here, that this render does not have his own 
		Render target resources and RTV heap for it. It uses SwapChain and BasicDXGI application RTV heap. So when we
		resize application, we do not need resize RT resources here, but we need resize DS Resources.
*/

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

