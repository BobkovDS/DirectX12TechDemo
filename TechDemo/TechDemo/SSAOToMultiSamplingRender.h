#pragma once
#include "RenderBase.h"
#include "PSODebugLayer_Screen.h"

class SSAOToMultiSamplingRender :
	public RenderBase
{	

	PSODebugLayer_Screen m_psoLayer;
	std::unique_ptr<Mesh> m_mesh;
	void build_screen();
	void build_TechDescriptors();
	DXGI_SAMPLE_DESC m_SampleDesc;
public:
	SSAOToMultiSamplingRender();
	~SSAOToMultiSamplingRender();

	void initialize(const RenderMessager& renderParams);
	void build();
	void draw(UINT textureID);
	void resize(UINT newWidth, UINT newHeight);
};

