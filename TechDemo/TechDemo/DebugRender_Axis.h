/*
	***************************************************************************************************
	Description:
		Render to draw Origin Axis in DEBUG draw mode

	***************************************************************************************************
*/

#pragma once
#include "RenderBase.h"
#include "PSODebugLayer_Axis.h"

class DebugRender_Axis :
	public RenderBase
{	
	std::unique_ptr<Mesh> m_axesMesh;
	PSODebugLayer_Axis m_psoLayer;

	void buildAxisGeometry();

public:
	DebugRender_Axis();
	~DebugRender_Axis();
		
	void build();
	void draw(UINT flag);
	void resize(UINT newWidth, UINT newHeight);	
};

