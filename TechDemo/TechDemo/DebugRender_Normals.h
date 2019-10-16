/*
	***************************************************************************************************
	Description:
		Render to draw Normal to object in DEBUG draw mode		

	***************************************************************************************************
*/
#pragma once
#include "RenderBase.h"
#include "PSODebugLayer_Normals.h"
class DebugRender_Normals :
	public RenderBase
{
	std::unique_ptr<Mesh> m_mesh;
	PSODebugLayer_Normals m_psoLayer;

public:
	DebugRender_Normals();
	~DebugRender_Normals();
		
	void build();
	void draw(UINT flag);
	void draw_layer(int layerID, int& instanseOffset, bool doDraw, D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology);
	void resize(UINT newWidth, UINT newHeight);	
};

