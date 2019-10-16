/*
	***************************************************************************************************
	Description:
		This render does final render for a scene, using results of any other Renders like SSAO, ShadowMap.
		It does in a normal way - just take all geometry, draws it with required shaders, using two packages of texture:
		Tech_textures(Other Renders results) and Textures. A key moment here, that this render does not have its own
		Render target resources and RTV heap for it. It uses SwapChain/MSAARenderTargets and BasicDXGI application RTV heap. So when we
		resize application, we do not need resize RT resources here, but we need resize DS Resources.

	***************************************************************************************************
*/

#pragma once
#include "RenderBase.h"
#include "PSOFinalRenderLayer.h"

class FinalRender :
	public RenderBase
{	
	ComPtr<ID3D12Resource>* m_swapChainResources;
	PSOFinalRenderLayer m_psoLayer;	

	UINT m_trianglesDrawnCount; // How much triangles actually were send for drawing
	UINT m_trianglesCountIfWithoutLOD; // How much triangles would be drawn without using LOD
	UINT m_trianglesCountInScene; //How much triangles in Scene at all (Without LOD and Frustum Culling)

	void build_SkyDescriptor();
	void draw_layer(int layerID, int& instanceOffset, bool doDraw);
public:
	FinalRender();
	~FinalRender();
	
	void build();
	void draw(UINT flag);	
	void resize(UINT newWidth, UINT newHeight);
	void setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources);
	UINT getTrianglesDrawnCount() {	return m_trianglesDrawnCount; }
	UINT getTrianglesCountIfWithoutLOD() {	return m_trianglesCountIfWithoutLOD; }
	UINT getTrianglesCountInScene() {	return m_trianglesCountInScene; }
};

