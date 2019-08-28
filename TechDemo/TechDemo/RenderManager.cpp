#include "RenderManager.h"

#define TECHSRVCOUNT 10 // see also in ResourceManager.cpp 

RenderManager::RenderManager(): m_initialized(false)
{
	m_debugMode = false;
	m_debug_Axes = true;
	m_debug_Lights = false;
	m_debug_Normals_Vertex = false;
	m_debug_View = false;
	m_isSSAOUsing = false;
	m_isShadowUsing = false;
	m_isNormalMappingUsing = false;
	m_renderMode = (1 << RM_FINAL);	
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
	m_frameResourceManager= renderParams.commonRenderData.FrameResourceMngr;
	m_texturesDescriptorHeap = renderParams.SRVHeap;
	m_finalRender.initialize(renderParams.commonRenderData);
	m_ssaoRender.initialize(renderParams.commonRenderData);	
	m_debugRenderAxes.initialize(renderParams.commonRenderData);
	m_debugRenderLights.initialize(renderParams.commonRenderData);
	m_debugRenderNormals.initialize(renderParams.commonRenderData);
	m_debugRenderScreen.initialize(renderParams.commonRenderData);	
	m_debugRenderView.initialize(renderParams.commonRenderData);

	RenderMessager lRenderParams = renderParams.commonRenderData;	
	lRenderParams.Width = 4096; 
	lRenderParams.Height = 4096;
	m_shadowRender.initialize(lRenderParams);

	lRenderParams = renderParams.commonRenderData;
	lRenderParams.Width /= 2; // We think that SSAO gives us Map twice size less
	lRenderParams.Height/= 2;
	lRenderParams.RTResourceFormat = SSAORender::AOMapFormat;
	m_blurRender.initialize(lRenderParams);

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

	// build Debug View Render
	m_debugRenderView.set_DescriptorHeap_RTV(m_applicationRTVHeap);
	m_debugRenderView.set_DescriptorHeap_DSV(m_finalRender.get_dsvHeapPointer());
	m_debugRenderView.build();
	m_debugRenderView.setSwapChainResources(m_swapChainResources);

	// build Debug Normals Render
	m_debugRenderNormals.set_DescriptorHeap_RTV(m_applicationRTVHeap);
	m_debugRenderNormals.set_DescriptorHeap_DSV(m_finalRender.get_dsvHeapPointer());
	m_debugRenderNormals.build();
	m_debugRenderNormals.setSwapChainResources(m_swapChainResources);

	// build Debug Screen Render
	m_debugRenderScreen.set_DescriptorHeap_RTV(m_applicationRTVHeap);
	m_debugRenderScreen.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	m_debugRenderScreen.build();
	m_debugRenderScreen.setSwapChainResources(m_swapChainResources);

	// build SSAO Render
	m_ssaoRender.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	m_ssaoRender.build();

	// build Blur Render	
	m_blurRender.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	m_blurRender.setInputResource(m_ssaoRender.getAOResource());
	m_blurRender.build(1);

	// build Shadow Render
	m_shadowRender.set_DescriptorHeap(m_texturesDescriptorHeap);
	m_shadowRender.build();	
}

void RenderManager::draw()
{
	assert(m_initialized);

	int lResourceIndex = m_swapChain->GetCurrentBackBufferIndex();

	int updatedFlags = 0;

	bool lFinalRender = true;

	int lTechnicBits = (m_isSSAOUsing << RTB_SSAO) | (m_isShadowUsing << RTB_SHADOWMAPPING) | 
		(m_isNormalMappingUsing << RTB_NORMALMAPPING);

	updatedFlags |= lTechnicBits;

	// SSAO 
	if (m_isSSAOUsing)
	{		
		m_ssaoRender.draw(updatedFlags);			
		
		if (m_renderMode & (1 << RM_SSAO_MAP1))
		{
		/* we do not copy in this because dest_resource_format and source_resource_format are not equal

		m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainResources[lResourceIndex].Get(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_COPY_DEST));
			m_cmdList->CopyResource(m_swapChainResources[lResourceIndex].Get(), m_ssaoRender.getVNResource());

			m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainResources[lResourceIndex].Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_PRESENT));*/
			m_debugRenderScreen.draw(2); // AO Map from SSAO render
		}
		else if (m_renderMode & (1 << RM_SSAO_MAP2))
				m_debugRenderScreen.draw(4); // AO Map from SSAO render
		else if (m_renderMode & (1 << RM_SSAO_MAP3) && m_blurRender.isBlurMapReady())
			m_debugRenderScreen.draw(5); // Blured AO Map from Blur render
	}
	
	if (m_isShadowUsing)
	{
		m_shadowRender.draw(0);
		if (m_renderMode & (1 << RM_SHADOW))
			m_debugRenderScreen.draw(6);
	}		

	updatedFlags &= ~((!m_blurRender.isBlurMapReady()) << RTB_SSAO);
	if (lFinalRender && !(m_renderMode & RM_OTHERMODE)) m_finalRender.draw(updatedFlags);
	   
	if (m_debugMode)
	{
		if (m_debug_Axes)
			m_debugRenderAxes.draw(updatedFlags);
		if (m_debug_Lights)
			m_debugRenderLights.draw(updatedFlags);
		if (m_debug_Normals_Vertex)
			m_debugRenderNormals.draw(updatedFlags);
		if (m_debug_View) m_debugRenderView.draw(updatedFlags);
	}

	// for using with GUI Render
	m_cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainResources[lResourceIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	if (m_isSSAOUsing)
	{
		m_blurRender.draw(0);
	}
}

void RenderManager::buildTechSRVs()
{
	/*
		Add [TECHSRVCOUNT] 'NULL' SRVs for technical Textures (Results for other Renders) in the beginning of
		common DescriptorHeap (m_texturesDescriptorHeap). Later each Render will use his slot(s) and will create 'normal' SRV
		0 - Sky Cube map
		1 - Random Vector Map
		2 - SSAO: View Normal Map
		3 - SSAO: Depth Map
		4 - SSAO: AO Map
		5 - BLUR: Resource_A_SRV (Blur Output result)
		6 - BLUR: Resource_B_UAV
		7 - BLUR: Resource_B_SRV
		8 - BLUR: Resource_A_UAV
		9 - Shadow Map
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
		m_ssaoRender.resize(iwidth, iheight);

		m_blurRender.setInputResource(m_ssaoRender.getAOResource());
		m_blurRender.resize(iwidth, iheight);
		//m_shadowRender.resize(iwidth, iheight); // we do not need resize Shadow Map everytimes
		m_finalRender.resize(iwidth, iheight);
		m_debugRenderAxes.resize(iwidth, iheight);
		m_debugRenderLights.resize(iwidth, iheight);		
		m_debugRenderNormals.resize(iwidth, iheight);	
		m_debugRenderScreen.resize(iwidth, iheight);
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

void RenderManager::toggleDebug_View()
{
	if (m_debugMode)
		m_debug_View = !m_debug_View;
}

void RenderManager::setRenderMode_Final()
{
	m_renderMode = (1 << RM_FINAL);
}

void RenderManager::setRenderMode_SSAO_Map1()
{
	m_renderMode = (1 << RM_SSAO_MAP1);
}

void RenderManager::setRenderMode_SSAO_Map2()
{
	m_renderMode = (1 << RM_SSAO_MAP2);
}

void RenderManager::setRenderMode_SSAO_Map3()
{
	m_renderMode = (1 << RM_SSAO_MAP3);
}

void RenderManager::setRenderMode_Shadow(UINT mapID)
{
	m_renderMode = (1 << RM_SHADOW);
}

void RenderManager::toggleTechnik_SSAO()
{
	m_isSSAOUsing = !m_isSSAOUsing;
}

void RenderManager::toggleTechnik_Shadow()
{
	m_isShadowUsing= !m_isShadowUsing;
}

void RenderManager::toggleTechnik_Normal()
{
	m_isNormalMappingUsing= !m_isNormalMappingUsing;
}