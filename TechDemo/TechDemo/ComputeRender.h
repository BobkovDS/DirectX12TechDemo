/*
	***************************************************************************************************
	Description:
		This render does compute work on Compute Shader to calculate water simulation. It implements Fluid simulation algoritm, 
		described in "Mathematics for 3D Programming and Compute Graphics, 3rd Edition", Eric Lengyel.

		- On build step, It takes WaterV2 object ID, get data for this object from Scene object, get Vertices data, fill with it InputTexture.
		- On Compute (every frame) work, Compute shader take this Input texture, does calculation, using two intermediate UAVs, and store data to Input Resource, 
		for using it by FinalRender to draw WaterV2 object. After building step, no data transfer between CPU and GPU.

	***************************************************************************************************
*/

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
	
	void build();
	void build(int objectID = 0); // based on wich WaterV2 object create this Render (one ComputeRender object for each WaterV2 object)
	void draw(UINT flag);	
	void drop();	
};

