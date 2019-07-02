#pragma once
#include "RenderBase.h"
#include "FinalRender.h"
#include "DebugRender_Axis.h"
#include "DebugRender_Light.h"
#include "DebugRender_Normals.h"


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
	DebugRender_Axis m_debugRenderAxes;
	DebugRender_Light m_debugRenderLights;
	DebugRender_Normals m_debugRenderNormals;
	
	bool m_debugMode;
	bool m_debug_Axes;
	bool m_debug_Lights;
	bool m_debug_Normals_Vertex;

	void buildTechSRVs();

public:
	RenderManager();
	~RenderManager();

	void initialize(const RenderManagerMessanger& renderParams);
	
	void buildRenders();
	void draw(int flags);
	void resize(int newWidth, int newHeight);	

	void toggleDebugMode();
	void toggleDebug_Normals_Vertex();
	void toggleDebug_Axes();
	void toggleDebug_Lights();
	bool isDebugMode() { return m_debugMode; }
};

