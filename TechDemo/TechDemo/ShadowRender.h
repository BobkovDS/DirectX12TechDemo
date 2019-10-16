/*
	***************************************************************************************************
	Description:
		This render implement dynamic shadow from directional (Orthogonal) light source.

	***************************************************************************************************
*/

#pragma once
#include "RenderBase.h"
#include "PSOShadowLayer.h"
#include "Timer.h"
 
class ShadowRender :
	public RenderBase
{
	PSOShadowLayer m_psoLayer;
	void build_TechDescriptors();
	void draw_layer(int layerID, int& instanseOffset, bool doDraw);
	Timer m_timer;
	void resize(UINT newWidth, UINT newHeight) = delete;

public:
	ShadowRender();
	~ShadowRender();
		
	void build();
	void draw(UINT flag);	
};

