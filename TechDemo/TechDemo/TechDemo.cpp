#include "TechDemo.h"
#include "ApplLogger.h"
#include <thread>

using namespace DirectX;

TechDemo::TechDemo(HINSTANCE hInstance, const std::wstring& applName, int width, int height)
	:BasicDXGI(hInstance, applName, width, height)	
{
	m_init3D_done = false;
	m_isTechFlag = false;
	m_isCameraManualControl = true;
	m_bigHUD = false;		
}
TechDemo::~TechDemo()
{
	FlushCommandQueue();
	if (m_defaultCamera)
		delete m_camera;
}

void TechDemo::onMouseDown(WPARAM btnState, int x, int y)
{
	m_mouseDown = true;
	m_mouseDownPoint.x = x;
	m_mouseDownPoint.y = y;	
}

void TechDemo::onMouseUp(WPARAM btnState, int x, int y)
{
	m_mouseDown = false;
}

void TechDemo::onMouseMove(WPARAM btnState, int x, int y)
{
	if (m_mouseDown)
	{

		if (btnState == MK_LBUTTON)
		{
			float dx = XMConvertToRadians(0.25f*static_cast<float>(x - m_mouseDownPoint.x));
			float dy = XMConvertToRadians(0.25f*static_cast<float>(y - m_mouseDownPoint.y));

			if (m_isCameraManualControl)
			{
				m_camera->pitch(dy);
				m_camera->rotateY(dx);
			}
		}
		else
			//if CTRL
			if (btnState == (MK_LBUTTON | MK_CONTROL))
			{
			}
			else if (btnState == (MK_RBUTTON | MK_CONTROL))
			{
			}

		m_mouseDownPoint.x = x;
		m_mouseDownPoint.y = y;
	}
}

void TechDemo::onKeyDown(WPARAM btnState)
{
	// other keyspressing

	if (GetAsyncKeyState('0') & 0x8000) // To limit FPS to <120
		toLimitFPS = !toLimitFPS;	

	switch (btnState)
	{
	case VK_SPACE:
	{		
		m_animationTimer.tt_Pause();		
	}
	break;
	case 'L': m_scene.toggleLightAnimation(); break;						// Turn on/off Light animation
	case 'C': m_isCameraManualControl = !m_isCameraManualControl;  break;	// Turn on/off Camera animation
	case 'Z': m_bigHUD = !m_bigHUD;  break;									// Change the size for HUG (Normal/Big)
	case VK_NUMPAD0: m_renderManager.toggleDebugMode(); break;				// Turn on/off Debug Mode (some debug visualization)
	case VK_NUMPAD1: m_renderManager.toggleDebug_Axes(); break;						// Turn on/off Axes drawing in Debug mode
	case VK_NUMPAD2: m_renderManager.toggleDebug_Lights(); break;					// Turn on/off Ligh direction drawing in Debug mode
	case VK_NUMPAD3: m_renderManager.toggleDebug_Normals_Vertex(); break;			// Turn on/off Normal drawing in Debug mode	for some objects layers
	case VK_NUMPAD5: m_renderManager.water_drop(); break;					// Add drop to Water
	case VK_NUMPAD7: m_renderManager.toggleTechnik_SSAO(); break;			// Turn on/off SSAO technique using
	case VK_NUMPAD8: m_renderManager.toggleTechnik_Shadow(); break;			// Turn on/off Dynamic Shadows technique using
	case VK_NUMPAD9: m_renderManager.toggleTechnik_ComputeWork(); break;	// Turn on/off Compute work for the water simulation
	case VK_NUMPAD4: m_renderManager.toggleTechnik_Reflection(); break;		// Turn on/off water reflection
	case VK_NUMPAD6: m_renderManager.toggleTechnik_Normal(); break;			// Turn on/off Normal Mapping technique

	// Which Image to show:
	case '1': m_renderManager.setRenderMode_Final(); break;			// Image from Final Render
	case '2': m_renderManager.setRenderMode_SSAO_Map1(); break;		// Image from SSAO Render: View-space normal map
	case '3': m_renderManager.setRenderMode_SSAO_Map2(); break;		// Image from SSAO Render: AO map
	case '4': m_renderManager.setRenderMode_SSAO_Map3(); break;		// Image from Blur Render: AO blured map
	case '5': m_renderManager.setRenderMode_Shadow(); break;		// Image from Shadow Render: Dynamic shadows depth map
	default:
		break;
	}
}

void TechDemo::init3D()
{
	BasicDXGI::init3D();
	//here we come with closed m_cmdList
	HRESULT res;
		
	// create_DSV has closed m_cmdList, lets open it again
	res = m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);
	assert(SUCCEEDED(res));

	Utilit3D::initialize(m_device.Get(), m_cmdList.Get());

	// Load Landscape
	{		
		FBXFileLoader m_fbx_loader;
		m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
		m_fbx_loader.loadSceneFile("Models\\Scene.fbx");
	}
	
	// Load lights
	{		
		FBXFileLoader m_fbx_loader;
		m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
		m_fbx_loader.loadSceneFile("Models\\Lights.fbx");
	}	

	// Load a Camera
	{		
		FBXFileLoader m_fbx_loader;
		m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
		m_fbx_loader.loadSceneFile("Models\\Camera.fbx");
	} 
		
	m_skeletonManager.evaluateAnimationsTime();	
	m_resourceManager.loadTexture();	
	m_resourceManager.loadMaterials();

	RenderManagerMessanger lRenderManagerParams;
	lRenderManagerParams.RTVHeap = m_rtvHeap.Get();
	lRenderManagerParams.RTResources = nullptr;	
	lRenderManagerParams.commonRenderData.Device = m_device.Get();
	lRenderManagerParams.commonRenderData.CmdList= m_cmdList.Get();	
	lRenderManagerParams.commonRenderData.RTResourceFormat = backBufferFormat();
	lRenderManagerParams.commonRenderData.DSResourceFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	lRenderManagerParams.commonRenderData.Width = width();
	lRenderManagerParams.commonRenderData.Height = height();
	lRenderManagerParams.commonRenderData.Scene = &m_scene;
	lRenderManagerParams.commonRenderData.FrameResourceMngr = &m_frameResourceManager;
	lRenderManagerParams.commonRenderData.ResourceMngr = &m_resourceManager;
	lRenderManagerParams.SRVHeap = m_resourceManager.getTexturesSRVDescriptorHeap();
		
	int lBonesCount = 100;
	m_frameResourceManager.Initialize(m_device.Get(), m_fence.Get(), m_objectManager.getCommonInstancesCount(),
		PASSCONSTBUFCOUNT, SSAOCONSTBUFCOUNT, lBonesCount);
	
	// Camera
	if (m_objectManager.getCameras().size() == 0)
		build_defaultCamera();	
	else
	{
		m_defaultCamera = false;
		m_camera =m_objectManager.getCameras()[0].get();
		PerspectiveCameraLens* lPerspectiveLens = dynamic_cast<PerspectiveCameraLens*> (m_camera->lens);
		float w = static_cast<float>(width());
		float h = static_cast<float>(height());
		float aspect = w / h;

		lPerspectiveLens->setAspectRatio(aspect);				
	}

	// When camera has beed updated we let a Scene to know about this (Scene is updated only when camera has been updated)
	ExecuterVoidVoid<Scene>* sceneCameraListener =
		new ExecuterVoidVoid<Scene>(&m_scene, &Scene::cameraListener);
	m_camera->addObserver(sceneCameraListener);
		
	ApplLogger::getLogger().log("TechDemo::init3D()::Scene building...", 0);	
	m_scene.build(&m_objectManager, m_camera, &m_skeletonManager);
	ApplLogger::getLogger().log("TechDemo::init3D()::Scene building is done", 0);
		
	m_renderManager.initialize(lRenderManagerParams);
	m_renderManager.buildRenders();			

	ApplLogger::getLogger().log("TechDemo::init3D()::before Cmd list execution.", 0);
	m_cmdList->Close();
	ID3D12CommandList* cmdsList[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdsList);
	ApplLogger::getLogger().log("TechDemo::init3D()::Cmd list execution is done.", 0);

	FlushCommandQueue();
	ApplLogger::getLogger().log("TechDemo::init3D()::Flush commands is done.", 0);

	m_resourceManager.releaseTexturesUploadHeaps();
	m_init3D_done = true;
	
	build_OffsetVectors();

	m_animationTimer.tt_RunStop();
		
	m_betweenFramesTimer.begin();
	ApplLogger::getLogger().log("TechDemo::init3D() is done", 0);
}

void TechDemo::update()
{
	m_animationTimer.tick();
	float lTickTime = m_animationTimer.deltaTime();

	m_scene.update();
	m_scene.updateLight(lTickTime/5);
	update_camera();	
	update_objectCB();
	update_BoneData();
	update_passCB();	
	update_passSSAOCB();
}

void TechDemo::update_camera()
{
	if (m_isCameraManualControl) // Camera manual control is On
	{
		float dt = 0.01f;
		float da = XMConvertToRadians(0.1f);

		if (toLimitFPS)
		{
			dt *= 10.0f;
			da = XMConvertToRadians(0.1f*10.0f);
		}

		if (GetAsyncKeyState('W') & 0x8000)
			m_camera->walk(dt);

		if (GetAsyncKeyState('S') & 0x8000)
			m_camera->walk(-dt);

		if (GetAsyncKeyState('A') & 0x8000)
			m_camera->strafe(-dt);

		if (GetAsyncKeyState('D') & 0x8000)
			m_camera->strafe(dt);

		m_camera->updateViewMatrix();
	}
	else // Camera manual control is Off, we use Animation for Camera, if it does exist
	{
		int lSkeletonCount = m_skeletonManager.getSkeletonNodeAnimatedCount();
		if (lSkeletonCount)
		{
			float lTickTime = m_animationTimer.deltaTime()/15.0f;

			SkinnedData& lSkeleton = m_skeletonManager.getSkeletonNodeAnimated(0); /*TO_DO: now we use the first Skeleton. fix to get Skeleton by ID from Camera object*/
			XMFLOAT4X4 &lFinalMatrices = lSkeleton.getNodeTransform(lTickTime, 0);			

			m_camera->transform(lFinalMatrices);
		}
	}
}

void TechDemo::update_objectCB()
{	
	if (!m_scene.isInstancesDataUpdateRequred()) return;	
		
	UINT lInstancesCount = m_scene.getSelectedInstancesCount();	
	if (lInstancesCount == 0) return;
	auto lInstances = m_scene.getInstances();

	auto currCBObject = m_frameResourceManager.currentFR()->getObjectCB();	

	for (int i=0; i< lInstancesCount; i++)
		currCBObject->CopyData(i, *lInstances[i]);		

	m_scene.decrementInstancesReadCounter();	
}

void TechDemo::update_BoneData()
{		
	float lTickTime = m_animationTimer.deltaTime();

	//lTickTime = 0;
	auto currBoneCB = m_frameResourceManager.currentFR()->getBoneCB();
	float lBoneCBID= 0;

	int lSkeletonCount = m_skeletonManager.getSkeletonSkinnedAnimatedCount();
	for (int si = 0; si < lSkeletonCount; si++)
	{
		SkinnedData& lSkeleton = m_skeletonManager.getSkeletonSkinnedAnimated(si);
		const std::vector<DirectX::XMFLOAT4X4>& lFinalMatrices = lSkeleton.getFinalTransforms(lTickTime, 0);

		InstanceDataGPU ltmp;
		for (int i = 0; i < lFinalMatrices.size(); i++)		
			currBoneCB->CopyData(lBoneCBID++, lFinalMatrices[i]);			
	}
}

void TechDemo::update_passCB()
{
	auto mMainPassCB = m_frameResourceManager.tmpPassConsts;

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX view = m_camera->getView();
	XMMATRIX proj = m_camera->lens->getProj();
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX viewProjT = XMMatrixMultiply(viewProj, T);		
		
	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));	
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));	
	XMStoreFloat4x4(&mMainPassCB.ViewProjT, XMMatrixTranspose(viewProjT));	
		
	mMainPassCB.EyePosW = m_camera->getPosition3f();
	mMainPassCB.TotalTime = m_animationTimer.totalTime();	

	const std::vector<LightCPU>& lights = m_scene.getLights();

	// -- Find shadow box position and size	
	float lLenght = 50.0f; //38

	XMFLOAT3 lcameraPos = m_camera->getPosition3f();
	XMFLOAT4 lsceneCenter = m_scene.getSceneBB().Center;
	XMFLOAT3 lsceneExtents = m_scene.getSceneBB().Extents;
	float groundLevel = lsceneCenter.y - lsceneExtents.y / 2.0f;
	float lCameraHigh = lcameraPos.y - groundLevel;
	XMVECTOR lCameraDirection = m_camera->getLook();
	XMVECTOR lCameraDirectionInXZPlane = lCameraDirection;
	lCameraDirectionInXZPlane = XMVectorSetY(lCameraDirectionInXZPlane, 0.0f);
	lCameraDirectionInXZPlane = XMVector3Normalize(lCameraDirectionInXZPlane);

	lcameraPos.y -= lCameraHigh;
	XMVECTOR lCameraPosv = XMLoadFloat3(&lcameraPos);
	lCameraPosv = lCameraPosv + lCameraDirectionInXZPlane * lLenght; /* we draw Shadow in front of camera half-plane. 
	It works good when we look mainly forward, but not down from high distance*/
	XMStoreFloat3(&lcameraPos, lCameraPosv);	
		
	for (size_t i = 0; i < lights.size(); i++)
	{
		LightGPU& lLightGPU = mMainPassCB.Lights[i];
		const LightCPU& lLightCPU = lights[i];
		lLightGPU.Direction = lLightCPU.Direction;
		
		lLightGPU.Strength = XMFLOAT3(lLightCPU.Color.x * lLightCPU.Intensity, lLightCPU.Color.y * lLightCPU.Intensity, 
			lLightCPU.Color.z * lLightCPU.Intensity);

		lLightGPU.Position = lLightCPU.Position;
		lLightGPU.spotPower = lLightCPU.spotPower;
		lLightGPU.falloffStart = lLightCPU.falloffStart;
		lLightGPU.falloffEnd = lLightCPU.falloffEnd;
		lLightGPU.lightType = lLightCPU.lightType + 1; // 0 - is undefined type of light
		lLightGPU.turnOn = lLightCPU.turnOn;

		// build ViewProjT matrix for shadow technique
		if (lLightCPU.lightType == LightType::Directional)
		{
			MathHelper::buildSunOrthoLightProjection(lLightGPU.Direction, lLightGPU.ViewProj, lLightGPU.ViewProjT,
				lcameraPos, lLenght);

			float lAmbientColor = 0.2f + 0.3f * lLightCPU.Intensity;			
		}
	}
	
	auto currPassCB = m_frameResourceManager.currentFR()->getPassCB();
	currPassCB->CopyData(PASSCONSTBUF_ID_MAIN, mMainPassCB);

	update_passMirror(mMainPassCB);
}

void TechDemo::update_passMirror(PassConstantsGPU& passCB)
{
	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMVECTOR lMirrorPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); //xz plane
	XMMATRIX lMirrorM = XMMatrixReflect(lMirrorPlane);

	XMMATRIX view = m_camera->getView();
	view = lMirrorM * view;

	XMMATRIX proj = m_camera->lens->getProj();
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX viewProjT = XMMatrixMultiply(viewProj, T);

	XMStoreFloat4x4(&passCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&passCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&passCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&passCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&passCB.ViewProjT, XMMatrixTranspose(viewProjT));
	
	/*
		Other parametrs of passCB will stay the same, in partucular we do not need make Light direction reflection (It because we apply
		Reflection matrix on View Matrix, not on World of each Object Instance, so Normal is not changed)
	*/
	auto currPassCB = m_frameResourceManager.currentFR()->getPassCB();
	currPassCB->CopyData(PASSCONSTBUF_ID_MIRROR, passCB);

}

void TechDemo::update_passSSAOCB()
{
	// For SSAO
	auto mSSAOPassCB = m_frameResourceManager.tmpPassSSAOConsts;		
	
	XMMATRIX proj = m_camera->lens->getProj();	
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);

	XMStoreFloat4x4(&mSSAOPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mSSAOPassCB.InvProj, XMMatrixTranspose(invProj));

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);
	
	XMMATRIX view = m_camera->lens->getProj();
	XMMATRIX textProj = XMMatrixMultiply(view, T);

	XMStoreFloat4x4(&mSSAOPassCB.ProjTex, XMMatrixTranspose(view* T));
	std::copy(&m_offsets[0], &m_offsets[14], &mSSAOPassCB.OffsetVectors[0]);

	mSSAOPassCB.OcclusionRadius = 0.5f;
	mSSAOPassCB.OcclusionFadeStart = 0.2;
	mSSAOPassCB.OcclusionFadeEnd = 1.0f;
	mSSAOPassCB.SurfaceEpsilon = 0.05f;

	auto blurWeights = calcGaussWeights(2.5f);
	mSSAOPassCB.BlurWeight[0] = XMFLOAT4(&blurWeights[0]);
	mSSAOPassCB.BlurWeight[1] = XMFLOAT4(&blurWeights[4]);
	mSSAOPassCB.BlurWeight[2] = XMFLOAT4(&blurWeights[8]);

	mSSAOPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / width(), 1.0f / height());
	
	// Calculate Coefficients for ComputeRender (WaterV2 object)	
	float c = 4.0f;
	float t = 0.029f;
	float dx = 0.043f; // TO_DO: Check here
	float mu = 0.2f;

	//check the limit values 
	float c_max = dx * sqrtf(mu*t + 2) / (2 * t);
	if (c >= c_max) c = c_max - 0.01f;

	float f1 = c * c * t * t / (dx * dx);
	float f2 = 1.0f / (mu * t + 2.0f);

	float k1 = (4.0f - 8.0f * f1) * f2;
	float k2 = (mu * t - 2.0f) * f2;
	float k3 = 2.0f * f1 * f2;

	mSSAOPassCB.k1 = k1;
	mSSAOPassCB.k2 = k2;
	mSSAOPassCB.k3 = k3;

	// Write data to CB
	auto currSsaoCB = m_frameResourceManager.currentFR()->getSSAOCB();
	currSsaoCB->CopyData(0, mSSAOPassCB);
}

void TechDemo::work()
{		
	// here we begin new Graphic Frame
	m_frameResourceManager.getFreeFR(); // so we need new Frame resource

	// for graphic frame we use CommandAllocator from FR
	m_frameResourceManager.changeCmdAllocator(m_cmdList.Get(), nullptr);
	
	// Count how much time went to prepare and draw previous frame (CPU + GPU work)
	float lTempValue = m_betweenFramesTimer.end();
	if (m_betweenFramesTimer.tick1Sec()) // we want to output this info only every 1 second
		m_betweenFramesTime = lTempValue;
	m_betweenFramesTimer.begin();
	
	// begin to count how much time we need to prepare data and fill command list for current Frame (only CPU work)
	m_framePrepareAndDrawTimer.begin();

	// update data
	update();
	UINT lSwapChainCurrentBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	m_renderManager.setCurrentRTID(lSwapChainCurrentBufferIndex);
	
	// draw data
	m_renderManager.draw();

	// Copy Draw data from non-SwapChain Render Target to SwapChain Render Target
	{
		uint8_t lrtID = lSwapChainCurrentBufferIndex;
		ID3D12Resource* lCurrentRTResource = m_renderManager.getCurrentRenderTargetResource();

		D3D12_RESOURCE_DESC lDesc = lCurrentRTResource->GetDesc();

		m_cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(lCurrentRTResource,
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE));

		m_cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffers[lSwapChainCurrentBufferIndex].Get(),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST));

		m_cmdList->ResolveSubresource(
			m_swapChainBuffers[lSwapChainCurrentBufferIndex].Get(), 0,
			lCurrentRTResource, 0, backBufferFormat());

		m_cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(lCurrentRTResource,
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

#ifndef GUI_HUD
		m_cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffers[lrtID].Get(),
				D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT));
#else
		m_cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffers[lrtID].Get(),
				D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));
#endif
		
	}
	lTempValue = m_framePrepareAndDrawTimer.end();

	if (m_framePrepareAndDrawTimer.tick1Sec()) // we want to output this info only every 1 second
		m_framePreparationTime = lTempValue;

	// excute command queue
	m_cmdList->Close();
	ID3D12CommandList* CmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, CmdLists);

	// draw GUI, if it is required
#ifdef GUI_HUD
	renderUI();
#endif
		
	// Present SwapChain
	m_swapChain->Present(0, 0);
	m_frameResourceManager.currentFR()->setFenceValue(getFenceValue());
	setFence(); //new frame is done
}

void TechDemo::build_defaultCamera()
{
	m_camera = new Camera();
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(4.0, 5.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorSet(0.0, 0.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_camera->lookAt(pos, target, up);

	float w = static_cast<float>(width());
	float h = static_cast<float>(height());
	float aspect = w / h;
	m_camera->lens->setLens(0.25f*XM_PI, aspect, 1.0f, 100.0f);

	m_defaultCamera = true;
}

std::string TechDemo::addTextToWindow()
{
	int lInstanceCount = m_scene.getInstances().size();
	int lDrawInstancesID = m_scene.getSelectedInstancesCount();	

	UINT lTrianglesDrawnCount = m_renderManager.getTrianglesDrawnCount();
	UINT lTrianglesCountIfWithoutLOD = m_renderManager.getTrianglesCountIfWithoutLOD();
	UINT lTrianglesCountInScene = m_renderManager.getTrianglesCountInScene();

	float lPercent1 = (float)lTrianglesCountIfWithoutLOD / (float)lTrianglesCountInScene * 100.0f;
	float lPercent2 = (float)lTrianglesDrawnCount / (float)lTrianglesCountInScene * 100.0f;

	char lbP1[10];
	sprintf_s(lbP1, _TRUNCATE, "(%3.2f%%)", lPercent1);

	char lbP2[10];
	sprintf_s(lbP2, _TRUNCATE, "(%3.2f%%)", lPercent2);

	char lbFrameTime[15];
	sprintf_s(lbFrameTime, _TRUNCATE, "(%.2fms)", m_framePreparationTime * 1000.0f);

	char lbBetweenFrameTime[50];
	sprintf_s(lbBetweenFrameTime, _TRUNCATE, "%d (%.2fms)", (UINT)(1.0f / m_betweenFramesTime), m_betweenFramesTime * 1000.0f);

	// Common count of triangles in Scene
	std::string lsTrianglesCountInScene = std::to_string(lTrianglesCountInScene);
	auto lStrIterator = lsTrianglesCountInScene.end();
	for (int i = 3; i < lsTrianglesCountInScene.length();)
	{
		lStrIterator = lStrIterator - 3;
		lsTrianglesCountInScene.insert(lStrIterator, ' ');
		i += 4;
	}

	// Count of Triangles, if it would be drawn without LOD
	std::string lsTrianglesCountIfWithoutLOD = std::to_string(lTrianglesCountIfWithoutLOD);
	lStrIterator = lsTrianglesCountIfWithoutLOD.end();
	for (int i = 3; i < lsTrianglesCountIfWithoutLOD.length();)
	{
		lStrIterator = lStrIterator - 3;
		lsTrianglesCountIfWithoutLOD.insert(lStrIterator, ' ');
		i += 4;
	}

	// Count of triangles which was drawn in fact
	std::string lsTrianglesCountInFact = std::to_string(lTrianglesDrawnCount);
	lStrIterator = lsTrianglesCountInFact.end();
	for (int i = 3; i < lsTrianglesCountInFact.length();)
	{
		lStrIterator = lStrIterator - 3;
		lsTrianglesCountInFact.insert(lStrIterator, ' ');
		i += 4;
	}

	std::string lBetweenFrame = "FPS: ";
	std::string lFrame = "\rTime to prepare a frame (CPU work only): ";

	std::string text =
		lBetweenFrame + lbBetweenFrameTime
		+ lFrame + lbFrameTime
		+ "\rInstances in Scene: " + std::to_string(lInstanceCount)
		+ "\rInstances in View Frustum: " + std::to_string(lDrawInstancesID)
		+ "\rTriangles in Scene: " + lsTrianglesCountInScene + " (100%)"
		+ "\rTriangles w/t LOD: " + lsTrianglesCountIfWithoutLOD + lbP1
		+ "\rTriangles were drawn: " + lsTrianglesCountInFact + lbP2;

	if (m_renderManager.isDebugMode())
		text += "\rDEBUG ";


	return text;
}

void TechDemo::renderUI()
{	
	int lResourceIndex = m_swapChain->GetCurrentBackBufferIndex();
	D2D_SIZE_F lrtSize = m_HUDRenderTargets[lResourceIndex]->GetSize();
	D2D_RECT_F ltextRect = D2D1::RectF(0, 0, lrtSize.width, lrtSize.height);
	std::string outputText = addTextToWindow();
		
	std::wstring wtest(outputText.begin(), outputText.end());

	m_d3d11On12Device->AcquireWrappedResources(m_wrappedBackBuffers[lResourceIndex].GetAddressOf(), 1);

	m_HUDContext->SetTarget(m_HUDRenderTargets[lResourceIndex].Get());
	m_HUDContext->BeginDraw();
	m_HUDContext->SetTransform(D2D1::Matrix3x2F::Identity());

	IDWriteTextFormat* lTextParam = m_textFormat.Get();
	if (m_bigHUD)
		lTextParam = m_textFormatBig.Get();
	
	m_HUDContext->DrawTextW(
		wtest.c_str(),
		wtest.length(),
		lTextParam,
		ltextRect,
		m_HUDBrush.Get());

	HRESULT res = m_HUDContext->EndDraw();
	m_HUDContext->SetTarget(nullptr);
	m_d3d11On12Device->ReleaseWrappedResources(m_wrappedBackBuffers[lResourceIndex].GetAddressOf(), 1);
	m_d3d11Context->Flush();
}


void TechDemo::onReSize(int newWidth, int newHeight)
{	
	BasicDXGI::onReSize(newWidth, newHeight);	
	// Here we have cmdList is closed 

	if (m_camera)
	{
		float w = static_cast<float>(width());
		float h = static_cast<float>(height());
		float aspect = w / h;		

		PerspectiveCameraLens* lPerspectiveLens = dynamic_cast<PerspectiveCameraLens*> (m_camera->lens);
		lPerspectiveLens->setAspectRatio(aspect);				
	}	

	if (!m_init3D_done) return; // onResize call it before how 3D part was init

	HRESULT res = 0;
	res = m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);
	assert(SUCCEEDED(res));
	
	m_renderManager.resize(newWidth, newHeight);
	
	res = m_cmdList->Close();
	ID3D12CommandList* cmdsList[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdsList);
	FlushCommandQueue();
}

std::vector<float> TechDemo::calcGaussWeights(float sigma)
{
	float twoSigma2 = 2.0f*sigma*sigma;

	// Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
	// For example, for sigma = 3, the width of the bell curve is 
	int blurRadius = (int)ceil(2.0f * sigma);

	assert(blurRadius <= MaxBlurRadius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;

	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		float x = (float)i;

		weights[i + blurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + blurRadius];
	}

	// Divide by the sum so all the weights add up to 1.0.
	for (int i = 0; i < weights.size(); ++i)
	{
		weights[i] /= weightSum;
	}

	return weights;
}

void TechDemo::build_OffsetVectors()
{
	// Start with 14 uniformly distributed vectors.  We choose the 8 corners of the cube
// and the 6 center points along each cube face.  We always alternate the points on 
// opposites sides of the cubes.  This way we still get the vectors spread out even
// if we choose to use less than 14 samples.

// 8 cube corners
	m_offsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
	m_offsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	m_offsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
	m_offsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

	m_offsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
	m_offsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

	m_offsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
	m_offsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

	// 6 centers of cube faces
	m_offsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	m_offsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

	m_offsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	m_offsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

	m_offsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	m_offsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

	for (int i = 0; i < 14; ++i)
	{
		// Create random lengths in [0.25, 1.0].
		float s = MathHelper::RandF(0.25f, 1.0f);

		XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&m_offsets[i]));

		XMStoreFloat4(&m_offsets[i], v);
	}
}