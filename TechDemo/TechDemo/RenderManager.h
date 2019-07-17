#pragma once
#include "RenderBase.h"
#include "FinalRender.h"
#include "DebugRender_Axis.h"
#include "DebugRender_Light.h"
#include "DebugRender_Normals.h"
#include "DebugRender_Screen.h"
#include "SSAORender.h"
#include "BlurRender.h"
#include "ShadowRender.h"


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
	IDXGISwapChain3* m_swapChain;		
	ComPtr<ID3D12Resource>* m_swapChainResources;
	ID3D12DescriptorHeap* m_applicationRTVHeap;
	ID3D12DescriptorHeap* m_texturesDescriptorHeap;
	DXGI_FORMAT m_rtResourceFormat;
	UINT m_width;
	UINT m_height;		
	IFrameResourcesManager* m_frameResourceManager;	

	bool m_initialized;
	FinalRender m_finalRender;
	SSAORender m_ssaoRender;
	BlurRender m_blurRender;
	ShadowRender m_shadowRender;
	DebugRender_Axis m_debugRenderAxes;
	DebugRender_Light m_debugRenderLights;
	DebugRender_Normals m_debugRenderNormals;
	DebugRender_Screen m_debugRenderScreen;
	
	bool m_debugMode;
	bool m_debug_Axes;
	bool m_debug_Lights;
	bool m_debug_Normals_Vertex;
	bool m_isSSAOUsing;
	bool m_isShadowUsing;
	bool m_isNormalMappingUsing;

	UINT m_renderMode;	

	void buildTechSRVs();

public:
	RenderManager();
	~RenderManager();

	void initialize(const RenderManagerMessanger& renderParams);
	
	void buildRenders();
	void draw();
	void resize(int newWidth, int newHeight);	

	void toggleDebugMode();
	void toggleDebug_Normals_Vertex();
	void toggleDebug_Axes();
	void toggleDebug_Lights();
	void toggleTechnik_SSAO();
	void toggleTechnik_Shadow();
	void toggleTechnik_Normal();
	void setRenderMode_Final();
	void setRenderMode_SSAO_Map1(); // ViewNormal Map
	void setRenderMode_SSAO_Map2(); // AO Map
	void setRenderMode_SSAO_Map3(); // AO Blured Map
	void setRenderMode_Shadow(UINT mapID=0); // Shadow Map


	bool isDebugMode() { return m_debugMode; }
};

