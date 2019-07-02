#include "RenderManager.h"

#define TECHSRVCOUNT 10 // see also in ResourceManager.cpp 

RenderManager::RenderManager(): m_initialized(false)
{
	m_debugMode = false;
	m_debug_Axes = true;
	m_debug_Lights = false;
	m_debug_Normals_Vertex = false;
}

RenderManager::~RenderManager()
{
}

void RenderManager::initialize(const RenderManagerMessanger& renderParams)
{
	m_device = renderParams.commonRenderData.Device;
	m_cmdList= renderParams.commonRenderData.CmdList;
	m_swapChain= renderParams.commonRenderData.SwapChain;
	m_swapChainResources = renderParams.RTResources;
	m_applicationRTVHeap = renderParams.RTVHeap;
	m_width= renderParams.commonRenderData.Width;
	m_height= renderParams.commonRenderData.Height;
	m_rtResourceFormat= renderParams.commonRenderData.RTResourceFormat;	
	m_psoManager= renderParams.commonRenderData.PSOMngr;
	m_frameResourceManager= renderParams.commonRenderData.FrameResourceMngr;
	m_texturesDescriptorHeap = renderParams.SRVHeap;
	m_finalRender.initialize(renderParams.commonRenderData);
	m_debugRenderAxes.initialize(renderParams.commonRenderData);
	m_debugRenderLights.initialize(renderParams.commonRenderData);
	m_debugRenderNormals.initialize(renderParams.commonRenderData);
	m_initialized = true;
}

void RenderManager::buildRenders()
{
	buildTechSRVs();

	//build Final Render	
	m_finalRender.set_DescriptorHeap_RTV(m_applicationRTVHeap);	
	m_finalRender.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	m_finalRender.build();
	m_finalRender.setSwapChainResources(m_swapChainResources);
	

	// build Debug Axes Render
	m_debugRenderAxes.set_DescriptorHeap_RTV(m_applicationRTVHeap);
	m_debugRenderAxes.build();
	m_debugRenderAxes.setSwapChainResources(m_swapChainResources);

	// build Debug Light Render
	m_debugRenderLights.set_DescriptorHeap_RTV(m_applicationRTVHeap);
	m_debugRenderLights.build();
	m_debugRenderLights.setSwapChainResources(m_swapChainResources);

	// build Debug Normals Render
	m_debugRenderNormals.set_DescriptorHeap_RTV(m_applicationRTVHeap);
	m_debugRenderNormals.set_DescriptorHeap_DSV(m_finalRender.get_dsvHeapPointer());
	m_debugRenderNormals.build();
	m_debugRenderNormals.setSwapChainResources(m_swapChainResources);
}

void RenderManager::draw(int flags)
{
	assert(m_initialized);

	int updatedFlags = flags;
	m_finalRender.draw(updatedFlags);

	if (m_debugMode)
	{
		if (m_debug_Axes)
			m_debugRenderAxes.draw(updatedFlags);
		if (m_debug_Lights)
			m_debugRenderLights.draw(updatedFlags);
		if (m_debug_Normals_Vertex)
			m_debugRenderNormals.draw(updatedFlags);
	}
}

void RenderManager::buildTechSRVs()
{
	/*
		Add [TECHSRVCOUNT] 'NULL' SRVs for technical Textures (Results for other Renders) in the beginning of
		common DescriptorHeap (m_texturesDescriptorHeap). Later each Render will use his slot(s) and will create 'normal' SRV
		0 - Sky Cube map
		1 - 'NULL'
		2 - 'NULL'
		3 - 'NULL'
		4 - 'NULL'
		5 - 'NULL'
		6 - 'NULL'
		7 - 'NULL'
		8 - 'NULL'
		9 - 'NULL'
	*/
	assert(m_texturesDescriptorHeap);

	UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptor(m_texturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	for (int i = 0; i < TECHSRVCOUNT; i++)
	{
		m_device->CreateShaderResourceView(nullptr,	&srvDesc, lhDescriptor);
		lhDescriptor.Offset(1, lSrvSize);
	}
}

void RenderManager::resize(int iwidth, int iheight)
{
	if (m_initialized)
	{
		m_finalRender.resize(iwidth, iheight);
	}	
}

void RenderManager::toggleDebugMode()
{	
	m_debugMode = !m_debugMode;
}

void RenderManager::toggleDebug_Axes()
{
	if (m_debugMode)
		m_debug_Axes = !m_debug_Axes;
}

void RenderManager::toggleDebug_Lights()
{
	if (m_debugMode)
		m_debug_Lights = !m_debug_Lights;
}

void RenderManager::toggleDebug_Normals_Vertex()
{
	if (m_debugMode)
		m_debug_Normals_Vertex = !m_debug_Normals_Vertex;
}