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
public:
	ShadowRender();
	~ShadowRender();

	void initialize(const RenderMessager& renderParams);
	void build();
	void draw(int flag);
	void resize(UINT newWidth, UINT newHeight);	
};

