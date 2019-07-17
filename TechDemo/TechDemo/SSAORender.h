#pragma once
#include "RenderBase.h"
#include "PSOSSAOLayer1.h"
#include "PSOSSAOLayer2.h"

#define RESOURCEID_VN 0
#define RESOURCEID_AO 1

class SSAORender :
	public RenderBase
{
	PSOSSAOLayer1 m_psoLayer1;
	PSOSSAOLayer2 m_psoLayer2;
	std::unique_ptr<Mesh> m_mesh;
	D3D12_GPU_DESCRIPTOR_HANDLE m_RNDVectorMapHandle;
	D3D12_VIEWPORT m_viewPortHalf;
	D3D12_RECT m_scissorRectHalf;
	ResourceWithUploader m_randomVectorsTexture;

	static const DXGI_FORMAT m_viewNormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	void build_TechDescriptors();
	void build_screen();
	void build_randomVectorTexture();
	void draw_layer(int layerID, int& instanseOffset, bool doDraw);
public:
	
	static const DXGI_FORMAT AOMapFormat = DXGI_FORMAT_R16_UNORM;

	SSAORender();
	~SSAORender();

	void initialize(const RenderMessager& renderParams);
	void build();
	void draw(int flag);	
	void resize(UINT newWidth, UINT newHeight);	
	ID3D12Resource* getVNResource();
	ID3D12Resource* getAOResource();	
	
};

