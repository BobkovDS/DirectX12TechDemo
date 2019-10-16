/*
	***************************************************************************************************
	Description:
		This render does compute work on Compute Shader to blur SSAO AO map

	***************************************************************************************************
*/

#pragma once
#include "RenderBase.h"
#include "PSOBlurLayer.h"
#include "Timer.h"

class BlurRender :
	public RenderBase
{
	ID3D12Resource* m_inputResource;			
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_VerticalCall_Handle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_HorizontalCall_Handle;	

	PSOBlurLayer m_psoLayer;	
	Timer m_timer;
	bool m_horizontalBlurFlag;
	int m_blurCount;
	bool m_isBlurReady;

	void create_ResrourcesUAV(DXGI_FORMAT resourceFormat);	
	void build_TechDescriptors();	
public:
	BlurRender();
	~BlurRender();
		
	void build();
	void build(int blurCount=4);
	void draw(UINT flag);
	void resize(UINT newWidth, UINT newHeight);
	void setInputResource(ID3D12Resource* inputResource);
	bool isBlurMapReady() { return m_isBlurReady; }
};

