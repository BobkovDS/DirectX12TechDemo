#pragma once
#include "RenderBase.h"
#include "FinalRender.h"

struct RenderManagerMessanger {
	ID3D12DescriptorHeap* RTVHeap;
	ComPtr<ID3D12Resource>* RTResources;
	ID3D12DescriptorHeap* SRVHeap; // Textures SRV heap
	RenderMessager commonRenderData;
};

class RenderManager
{
	ID3D12Device* m_device;
	ID3D12GraphicsCommandList* m_cmdList;
	IDXGISwapChain* m_swapChain;	
	IDXGISwapChain3* m_swapChain3;	
	ComPtr<ID3D12Resource>* m_swapChainResources;
	ID3D12DescriptorHeap* m_applicationRTVHeap;
	ID3D12DescriptorHeap* m_texturesDescriptorHeap;
	DXGI_FORMAT m_rtResourceFormat;
	UINT m_width;
	UINT m_height;	
	PSOManager* m_psoManager;
	IFrameResourcesManager* m_frameResourceManager;	

	bool m_initialized;
	FinalRender m_finalRender;

	void buildTechSRVs();

public:
	RenderManager();
	~RenderManager();

	void initialize(const RenderManagerMessanger& renderParams);
	
	void buildRenders();
	void draw(int flags);
	void resize(int newWidth, int newHeight);
	void releaseExternalResources();
	void connectExternalResourcesBack();
};

