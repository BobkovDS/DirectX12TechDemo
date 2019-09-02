#pragma once
#include "RenderBase.h"
#include "PSOComputeLayer.h"
#include "Timer.h"

class ComputeRender :
	public RenderBase
{
	ResourceWithUploader m_inputResource;
	ID3D12DescriptorHeap* m_descriptorHeapCompute; 

	PSOComputeLayer m_psoLayer;
	std::unique_ptr<Mesh> m_mesh;
	Timer m_timer;
	bool m_bufferFlag;
	int m_ObjectID;
	int m_textureID;
	bool m_isReady;
	bool m_drop;

	void create_ResrourcesUAV(DXGI_FORMAT resourceFormat);
	void build_TechDescriptors();	
	void create_DescriptorHeap_Compute();
	void resize(UINT newWidth, UINT newHeight) = delete; // no need to be resized.
public:
	ComputeRender();
	~ComputeRender();

	void initialize(const RenderMessager& renderParams);
	void build(int objectID = 0); // based on wich Water object create this Render (one ComputeRender object for each WaterV2 object)
	void draw(int flag);	
	void drop();
	
};

