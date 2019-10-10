#include "RenderManager.h"


RenderManager::RenderManager(): m_initialized(false)
{
	m_debugMode = false;
	m_debug_Axes = true;
	m_debug_Lights = false;
	m_debug_Normals_Vertex = false;	
	m_isSSAOUsing = false;
	m_isShadowUsing = false;
	m_isNormalMappingUsing = false;
	m_isComputeWork = false;
	m_isReflection = false;
	m_renderMode = (1 << RM_FINAL);		
}

RenderManager::~RenderManager()
{
}

void RenderManager::initialize(RenderManagerMessanger& renderParams)
{
	if (renderParams.RTResources != nullptr) // Do we use SwapChain RenderTarget Buffers directly or through MSAA RenderTargets
		m_swapChainResources = renderParams.RTResources;
	else
		m_swapChainResources = m_msaaRenderTargets.RenderTargetBuffers;

	m_applicationRTVHeap = renderParams.RTVHeap;
	m_texturesDescriptorHeap = renderParams.SRVHeap;
	m_device = renderParams.commonRenderData.Device;
	m_cmdList= renderParams.commonRenderData.CmdList;
	m_width= renderParams.commonRenderData.Width;
	m_height= renderParams.commonRenderData.Height;
	m_rtResourceFormat= renderParams.commonRenderData.RTResourceFormat;		
	m_frameResourceManager= renderParams.commonRenderData.FrameResourceMngr;	

	FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };
	D3D12_CLEAR_VALUE optClear;
	optClear.Format = m_rtResourceFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	std::copy(clearColor, clearColor + 4, optClear.Color);

	m_msaaRenderTargets.initialize(m_device, m_width, m_height, m_rtResourceFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &optClear);
	m_msaaRenderTargets.resize(m_width, m_height);
	m_msaaRenderTargets.createRTV(m_applicationRTVHeap);

	renderParams.commonRenderData.MSAARenderTarget = &m_msaaRenderTargets;

	m_finalRender.initialize(renderParams.commonRenderData);
	m_mirrorRender.initialize(renderParams.commonRenderData);	
	m_ssaoMSRender.initialize(renderParams.commonRenderData);
	m_debugRenderAxes.initialize(renderParams.commonRenderData);
	m_debugRenderLights.initialize(renderParams.commonRenderData);
	m_debugRenderNormals.initialize(renderParams.commonRenderData);
	m_debugRenderScreen.initialize(renderParams.commonRenderData);	
	m_computeRender.initialize(renderParams.commonRenderData);	

	RenderMessager lRenderParams = renderParams.commonRenderData;	
	lRenderParams.Width = 4096; 
	lRenderParams.Height = 4096;
	m_shadowRender.initialize(lRenderParams);

	lRenderParams = renderParams.commonRenderData;
	//lRenderParams.Width /= 2; 
	//lRenderParams.Height /= 2;	
	m_ssaoRender.initialize(lRenderParams);
		
	lRenderParams.Width /= 2; 
	lRenderParams.Height /= 2;	
	lRenderParams.RTResourceFormat = SSAORender::AOMapFormat;
	m_blurRender.initialize(lRenderParams);

	lRenderParams = renderParams.commonRenderData;
	lRenderParams.Width = 1024; 
	lRenderParams.Height = 1024;
	m_dcmRender.initialize(lRenderParams);

	m_initialized = true;
}

void RenderManager::buildRenders()
{
	buildTechSRVs();

	//build Final Render	
	m_finalRender.set_DescriptorHeap_RTV(m_applicationRTVHeap);	
	m_finalRender.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	m_finalRender.build();
	//m_finalRender.setSwapChainResources(m_swapChainResources); // TO_DO: delete Swapchain from renders

	//build Mirror Render	
	m_mirrorRender.set_DescriptorHeap_RTV(m_applicationRTVHeap);
	m_mirrorRender.set_DescriptorHeap_DSV(m_finalRender.get_dsvHeapPointer());
	m_mirrorRender.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	//m_mirrorRender.setSwapChainResources(m_swapChainResources);
	m_mirrorRender.build();	

	// build Debug Axes Render
	m_debugRenderAxes.set_DescriptorHeap_RTV(m_applicationRTVHeap);
	m_debugRenderAxes.build();
	//m_debugRenderAxes.setSwapChainResources(m_swapChainResources);

	// build Debug Light Render
	m_debugRenderLights.set_DescriptorHeap_RTV(m_applicationRTVHeap);
	m_debugRenderLights.build();
	//m_debugRenderLights.setSwapChainResources(m_swapChainResources);

	// build Debug Normals Render
	m_debugRenderNormals.set_DescriptorHeap_RTV(m_applicationRTVHeap);
	m_debugRenderNormals.set_DescriptorHeap_DSV(m_finalRender.get_dsvHeapPointer());
	m_debugRenderNormals.build();
	//m_debugRenderNormals.setSwapChainResources(m_swapChainResources);

	// build Debug Screen Render
	m_debugRenderScreen.set_DescriptorHeap_RTV(m_applicationRTVHeap);
	m_debugRenderScreen.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	m_debugRenderScreen.build();
	//m_debugRenderScreen.setSwapChainResources(m_swapChainResources);

	// build SSAO Render
	m_ssaoRender.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	m_ssaoRender.build();

	// build SSAO Render
	m_ssaoMSRender.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	m_ssaoMSRender.build();

	// build Blur Render	
	m_blurRender.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	m_blurRender.setInputResource(m_ssaoRender.getAOResource());	
	m_blurRender.build(2);

	// build Shadow Render
	m_shadowRender.set_DescriptorHeap(m_texturesDescriptorHeap);
	m_shadowRender.build();	

	// build Compute Render
	m_computeRender.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	m_computeRender.build(0);

	// build SSAO Render
	m_dcmRender.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	m_dcmRender.build();
}

void RenderManager::draw()
{
	assert(m_initialized);	

	int updatedFlags = 0;

	bool lFinalRender = true;

	int lTechnicBits = (m_isSSAOUsing << RTB_SSAO) | (m_isShadowUsing << RTB_SHADOWMAPPING) | 
		(m_isNormalMappingUsing << RTB_NORMALMAPPING);

	updatedFlags |= lTechnicBits;

	// SSAO 
	if (m_isSSAOUsing)
	{		
		m_ssaoRender.draw(updatedFlags);			
		//m_ssaoMSRender.draw(TECHSLOT_BLUR_A_SRV);

		if (m_renderMode & (1 << RM_SSAO_MAP1))
		{
			/* we do not copy in this because dest_resource_format and source_resource_format are not equal	*/
			m_debugRenderScreen.draw(TECHSLOT_SSAO_VSN); // ViewSpace Normal Map from SSAO render
		}
		else if (m_renderMode & (1 << RM_SSAO_MAP2))
				m_debugRenderScreen.draw(TECHSLOT_SSAO_AO); // AO Map from SSAO render
		else if (m_renderMode & (1 << RM_SSAO_MAP3) && m_blurRender.isBlurMapReady())
			m_debugRenderScreen.draw(TECHSLOT_BLUR_A_SRV); // Blured AO Map from Blur render
	}
	
	if (m_isShadowUsing)
	{
		m_shadowRender.draw(0);
		if (m_renderMode & (1 << RM_SHADOW))
			m_debugRenderScreen.draw(TECHSLOT_SHADOW);
	}		

	updatedFlags &= ~((!m_blurRender.isBlurMapReady()) << RTB_SSAO);
	if (lFinalRender && !(m_renderMode & RM_OTHERMODE))
	{
		m_finalRender.draw(updatedFlags);
		
		if (m_isReflection)
			m_mirrorRender.draw(1);
		else
			m_mirrorRender.draw(0);
	}
	  
	//m_dcmRender.draw(updatedFlags);

	if (m_debugMode)
	{
		if (m_debug_Axes)
			m_debugRenderAxes.draw(updatedFlags);
		if (m_debug_Lights)
			m_debugRenderLights.draw(updatedFlags);
		if (m_debug_Normals_Vertex)
			m_debugRenderNormals.draw(updatedFlags);		
	}

	if (m_isSSAOUsing)
	{
		m_blurRender.draw(0);			
	}

	if (m_isComputeWork)
		m_computeRender.draw(0);
}

void RenderManager::buildTechSRVs()
{
	/*
		Add [TECHSRVCOUNT] 'NULL' SRVs for technical Textures (Results for other Renders) in the beginning of
		common DescriptorHeap (m_texturesDescriptorHeap). Later each Render will use his slot(s) and will create 'normal' SRV
		0 - Sky Cube map
		1 - Dynamic Cube Map
		2 - Random Vector Map
		3 - SSAO: View Normal Map
		4 - SSAO: Depth Map
		5 - SSAO: AO Map
		6 - BLUR: Resource_A_SRV (Blur Output result)
		7 - BLUR: Resource_B_UAV
		8 - BLUR: Resource_B_SRV
		9 - BLUR: Resource_A_UAV
		10 - Shadow Map
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

void RenderManager::setCurrentRTID(UINT current_swapChainBufferID)
{
	m_msaaRenderTargets.setCurrentBufferID((uint8_t)current_swapChainBufferID);
}

ID3D12Resource* RenderManager::getCurrentRenderTargetResource()
{
	uint8_t lCurrentRTIndex = m_msaaRenderTargets.getCurrentBufferID();
	return m_msaaRenderTargets.RenderTargetBuffers[lCurrentRTIndex].Get();
}

void RenderManager::resize(int iwidth, int iheight)
{
	if (m_initialized)
	{
		m_width = iwidth;
		m_height = iheight;
		m_msaaRenderTargets.resize(iwidth, iheight);
		m_msaaRenderTargets.createRTV(m_applicationRTVHeap);

		m_ssaoRender.resize(iwidth, iheight);
		m_ssaoMSRender.resize(iwidth, iheight);

		m_blurRender.setInputResource(m_ssaoRender.getAOResource());
		m_blurRender.resize(iwidth, iheight);		
		m_finalRender.resize(iwidth, iheight);
		m_mirrorRender.resize(iwidth, iheight);
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

void RenderManager::toggleTechnik_ComputeWork()
{
	m_isComputeWork= !m_isComputeWork;
}

void RenderManager::toggleTechnik_Reflection()
{
	m_isReflection = !m_isReflection;
}


void RenderManager::test_drop()
{
	m_computeRender.drop();
}
