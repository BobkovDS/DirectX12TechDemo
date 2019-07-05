#pragma once
#include "RenderBase.h"
#include "PSOBlurLayer.h"

class BlurRender :
	public RenderBase
{
	ID3D12Resource* m_inputResource;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_RTV_Handle1;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_RTV_Handle2;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_RTV_Current;	
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_SRV_Handle1;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_SRV_Handle2;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_SRV_Current;

	PSOBlurLayer m_psoLayer;
	std::unique_ptr<Mesh> m_mesh;
	bool m_horizontalBlurFlag;
	int m_blurCount;
	void create_DescriptorHeap_RTV();
	void build_TechDescriptors();
	void build_screen();
public:
	BlurRender();
	~BlurRender();

	void initialize(const RenderMessager& renderParams);
	void build(std::string shaderName, int blurCount=4);
	void draw(int flag);
	void resize(UINT newWidth, UINT newHeight);
	void setInputResource(ID3D12Resource* inputResource);
};

