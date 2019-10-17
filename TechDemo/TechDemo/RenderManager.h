/*
	***************************************************************************************************
	Description:
		Connects all Renders together. The task for RenderManager is to initialize all Renders, to create/set 
		visualization flags and to create a draw-flow based on it.

	***************************************************************************************************
*/

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
#include "ComputeRender.h"
#include "DCMRender.h"
#include "MirrorRender.h"

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
	MSAARenderTargets m_msaaRenderTargets;

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
	ComputeRender m_computeRender;
	DCMRender m_dcmRender;
	ShadowRender m_shadowRender;
	MirrorRender m_mirrorRender;
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
	bool m_isComputeWork;
	bool m_isReflection;
	UINT m_renderMode;	

	void buildTechSRVs();

public:
	RenderManager();
	~RenderManager();

	void initialize(RenderManagerMessanger& renderParams);
	
	void buildRenders();
	void draw();
	void resize(int newWidth, int newHeight);	
	void setCurrentRTID(UINT current_swapChainBufferID);
	void toggleDebugMode();
	void toggleDebug_Normals_Vertex();
	void toggleDebug_Axes();
	void toggleDebug_Lights();	
	void toggleTechnik_SSAO();
	void toggleTechnik_Shadow();
	void toggleTechnik_Reflection();
	void toggleTechnik_Normal();	
	void toggleTechnik_ComputeWork();	
	void setRenderMode_Final();
	void setRenderMode_SSAO_Map1(); // ViewNormal Map
	void setRenderMode_SSAO_Map2(); // AO Map
	void setRenderMode_SSAO_Map3(); // AO Blured Map
	void setRenderMode_Shadow(UINT mapID=0); // Shadow Map
	void water_drop();
	bool isDebugMode() { return m_debugMode; }
	ID3D12Resource* getCurrentRenderTargetResource();

	UINT getTrianglesDrawnCount() { return m_finalRender.getTrianglesDrawnCount(); } // How much Triangles were sent for drawing in FinalRender
	UINT getTrianglesCountIfWithoutLOD() { return m_finalRender.getTrianglesCountIfWithoutLOD(); } // How much triangles would be drawn without using LOD
	UINT getTrianglesCountInScene() { return m_finalRender.getTrianglesCountInScene(); } // How much triangles in Scene at all (Without LOD and Frustum Culling)
};