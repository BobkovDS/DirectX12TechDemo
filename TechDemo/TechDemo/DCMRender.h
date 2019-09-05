/*
	Dynamic Cube Map Render
*/

#pragma once
#include "RenderBase.h"
#include "PSODCMLayer.h"
#include "Timer.h"

class DCMRender :
	public RenderBase
{
	static const DXGI_FORMAT m_viewNormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	PSODCMLayer m_psoLayer;		
	D3D12_GPU_DESCRIPTOR_HANDLE m_RNDVectorMapHandle;	
	Timer m_timer;
			
	bool m_isFirstBuild;	

	void build_TechDescriptors();	
	void draw_layer(int layerID, int& instanseOffset, bool doDraw);
	void create_RTV(DXGI_FORMAT viewFormat = DXGI_FORMAT_UNKNOWN);
public:
	DCMRender();
	~DCMRender();

	void initialize(const RenderMessager& renderParams);
	void build();
	void draw(int flag);
	void resize(UINT newWidth, UINT newHeight);
};

