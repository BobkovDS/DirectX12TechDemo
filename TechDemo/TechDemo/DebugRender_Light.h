/*
	***************************************************************************************************
	Description:
		Render to draw Light Direction for the first Directional Light source in DEBUG draw mode

	***************************************************************************************************
*/

#pragma once
#include "RenderBase.h"
#include "PSODebugLayer_Light.h"
class DebugRender_Light :
	public RenderBase
{
	std::unique_ptr<Mesh> m_mesh;
	PSODebugLayer_Light m_psoLayer;

	void buildGeometry();
public:
	DebugRender_Light();
	~DebugRender_Light();
		
	void build();
	void draw(UINT flag);
	void resize(UINT newWidth, UINT newHeight);	
};

