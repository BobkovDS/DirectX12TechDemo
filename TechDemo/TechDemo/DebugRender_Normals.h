#pragma once
#include "RenderBase.h"
#include "PSODebugLayer_Normals.h"
class DebugRender_Normals :
	public RenderBase
{
	IDXGISwapChain3* m_swapChain;
	ComPtr<ID3D12Resource>* m_swapChainResources;

	std::unique_ptr<Mesh> m_mesh;
	PSODebugLayer_Normals m_psoLayer;

	void buildGeometry();
public:
	DebugRender_Normals();
	~DebugRender_Normals();
		
	void initialize(const RenderMessager& renderParams);
	void build();
	void draw(int flag);
	void draw_layer(int layerID, int& instanseOffset, bool doDraw, D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology);
	void resize(UINT newWidth, UINT newHeight);
	void setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources);
};

