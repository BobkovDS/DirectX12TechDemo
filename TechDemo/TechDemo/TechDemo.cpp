#include "TechDemo.h"
#include "ApplLogger.h"

using namespace DirectX;

TechDemo::TechDemo(HINSTANCE hInstance, const std::wstring& applName, int width, int height)
	:BasicDXGI(hInstance, applName, width, height)	
{
	m_init3D_done = false;
	m_isTechFlag = false;
	m_isCameraManualControl = true;
	//Utilit3D::nullitizator();

	m_drawInstancesIDs = new UINT[MaxInstancesCount];
}
TechDemo::~TechDemo()
{
	FlushCommandQueue();
	if (m_defaultCamera)
		delete m_camera;

	delete[] m_drawInstancesIDs;
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

		//if CTRL
		if (btnState == (MK_LBUTTON | MK_CONTROL))
		{
			//if (y != m_MouseDownPoint.y) mTheta += (y - m_MouseDownPoint.y)/abs(y - m_MouseDownPoint.y) *0.5f;

			//if (x != m_MouseDownPoint.x) mPhi += (m_MouseDownPoint.x - x)/abs(m_MouseDownPoint.x - x)* 0.5f;

		}
		else if (btnState == (MK_RBUTTON | MK_CONTROL))
		{
			//if (x != m_MouseDownPoint.x) mRadius += (m_MouseDownPoint.x - x)/abs(m_MouseDownPoint.x - x)* 1.0f;
		}

		else if (btnState == MK_LBUTTON)
		{
			float dx = XMConvertToRadians(0.25f*static_cast<float>(x - m_mouseDownPoint.x));
			float dy = XMConvertToRadians(0.25f*static_cast<float>(y - m_mouseDownPoint.y));

			m_camera->pitch(dy);
			m_camera->rotateY(dx);
		}
		m_mouseDownPoint.x = x;
		m_mouseDownPoint.y = y;
	}
}

void TechDemo::onKeyDown(WPARAM btnState)
{
	// other keyspressing

	if (GetAsyncKeyState('0') & 0x8000)
		toLimitFPS = !toLimitFPS;	

	switch (btnState)
	{
	case VK_SPACE:
	{
		//m_isTechFlag = !m_isTechFlag;
		m_animationTimer.tt_Pause();
		m_scene.toggleLightAnimation();
	}
	break;
	case 'L': m_isCameraManualControl = !m_isCameraManualControl; break;
	case 'C': m_scene.toggleCullingMode(); break;
	case VK_NUMPAD0: m_renderManager.toggleDebugMode(); break;
	case VK_NUMPAD1: m_renderManager.toggleDebug_Axes(); break;
	case VK_NUMPAD2: m_renderManager.toggleDebug_Lights(); break;
	case VK_NUMPAD3: m_renderManager.toggleDebug_Normals_Vertex(); break;	
	case VK_NUMPAD5: m_renderManager.test_drop(); break;
	case VK_NUMPAD7: m_renderManager.toggleTechnik_SSAO(); break;
	case VK_NUMPAD8: m_renderManager.toggleTechnik_Shadow(); break;
	case VK_NUMPAD9: m_renderManager.toggleTechnik_ComputeWork(); break;
	case VK_NUMPAD4: m_renderManager.toggleTechnik_Reflection(); break;
	case VK_NUMPAD6: m_renderManager.toggleTechnik_Normal(); break;

	case '1': m_renderManager.setRenderMode_Final(); break;
	case '2': m_renderManager.setRenderMode_SSAO_Map1(); break;
	case '3': m_renderManager.setRenderMode_SSAO_Map2(); break;
	case '4': m_renderManager.setRenderMode_SSAO_Map3(); break;
	case '5': m_renderManager.setRenderMode_Shadow(); break;
	default:
		break;
	}
}

std::string TechDemo::addTextToWindow()
{
	int lInstanceCount = m_scene.getInstances().size();
	int lDrawInstancesID = m_scene.getSelectedInstancesCount();
	bool lOctreCullingMode = m_scene.getCullingModeOctree();

	UINT lTrianglesDrawnCount = m_renderManager.getTrianglesDrawnCount();
	UINT lTrianglesCountIfWithoutLOD = m_renderManager.getTrianglesCountIfWithoutLOD();
	UINT lTrianglesCountInScene = m_renderManager.getTrianglesCountInScene();

	float lPercent1 = (float) lTrianglesCountIfWithoutLOD / (float) lTrianglesCountInScene * 100.0f;
	float lPercent2 = (float) lTrianglesDrawnCount / (float) lTrianglesCountInScene * 100.0f;
	
	char lbP1[10];
	_snprintf_s(lbP1, sizeof(lbP1), "%3.1f", lPercent1);
	std::string lsP1 = lbP1;
	lsP1 = " (" + lsP1 + "%)";

	char lbP2[10];
	_snprintf_s(lbP2, sizeof(lbP2), "%3.1f", lPercent2);
	std::string lsP2 = lbP2;
	lsP2 = " (" + lsP2 + "%)";

	std::string text = "InstCount(Shadow): " + std::to_string(lInstanceCount)
		+ "\rInstCount(Drawing): " + std::to_string(lDrawInstancesID)
		+ "\rContains count: " + std::to_string(m_scene.ContainsCount)
		+ "\rTriangles in Scene count: " + std::to_string(lTrianglesCountInScene) + " (100%)"
		+ "\rTriangles would be w/t LOD count: " + std::to_string(lTrianglesCountIfWithoutLOD) + lsP1
		+ "\rTriangles were drawn count: " + std::to_string(lTrianglesDrawnCount) + lsP2;
	
	if (m_isTechFlag)
		text += "\rTechFlag=1";
	else
		text += "\rTechFlag=0";

	if (m_isCameraManualControl)
		text += "\rCameraMC=1";
	else
		text += "\rCameraMC=0";

	if (lOctreCullingMode)
		text += "\rOctreeMode=1";
	else
		text += "\rOctreeMode=0";

	if (m_renderManager.isDebugMode())
		text += "\rDEBUG ";

	
	return text;
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

	//// Load Wolf with Animation
	//{
	//	FBXFileLoader m_fbx_loader;
	//	m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
	//	m_fbx_loader.loadSceneFile("Models\\Wolf.fbx");
	//}

	//{
	//	FBXFileLoader m_fbx_loader;
	//	m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
	//	m_fbx_loader.loadSceneFile("Models\\Water3.fbx");
	//}

	//Load a house
	{
		FBXFileLoader m_fbx_loader;
		m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
		m_fbx_loader.loadSceneFile("Models\\The Scene.fbx");		
	}

	{
		FBXFileLoader m_fbx_loader;
		m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
		m_fbx_loader.loadSceneFile("Models\\Landscape.fbx");
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
	lRenderManagerParams.RTResources = m_swapChainBuffers;
	lRenderManagerParams.commonRenderData.Device = m_device.Get();
	lRenderManagerParams.commonRenderData.CmdList= m_cmdList.Get();
	lRenderManagerParams.commonRenderData.SwapChain = m_swapChain.Get();	
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
		PASSCONSTBUFCOUNT, SSAOCONSTBUFCOUNT, lBonesCount, MaxInstancesCount);
	
	//build_defaultCamera();

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
		m_camera->buildFrustumBounding();		
	}

	// When camera has beed updated we let a Scene to know about this (Scene is updated only when camera has been updated)
	ExecuterVoidVoid<Scene>* sceneCameraListener =
		new ExecuterVoidVoid<Scene>(&m_scene, &Scene::cameraListener);
	m_camera->addObserver(sceneCameraListener);
	
	//m_objectManager.mirrorZ();
	ApplLogger::getLogger().log("TechDemo::init3D()::Scene building...", 0);
	m_scene.build(&m_objectManager, m_camera, &m_skeletonManager);
	ApplLogger::getLogger().log("TechDemo::init3D()::Scene building is done", 0);

	m_renderManager.initialize(lRenderManagerParams);
	m_renderManager.buildRenders();		

	// Build Cameras for DCM	
	//if (PASSCONSTBUF_COUNT_DCM > 0)
	//{
	//	XMFLOAT3 position(0.0f, 0.0f, 0.0f); // Get Position of a Lake

	//	// Get the first WaveV2 object position
	//	Scene::SceneLayer* lWaveV2Layer = nullptr;
	//	lWaveV2Layer = m_scene.getLayer(NOTOPAQUELAYERCH);
	//	if (lWaveV2Layer)
	//	{
	//		Scene::SceneLayer::SceneLayerObject* lWaveV2Object = lWaveV2Layer->getSceneObject(0);
	//		if (lWaveV2Object)
	//		{
	//			XMMATRIX lWorld = XMLoadFloat4x4(&lWaveV2Object->getObjectMesh()->Instances[0].World);
	//			XMVECTOR lPosition = XMVectorSet(0.0f, 0.0f, 0.0, 1.0f);
	//			lPosition = XMVector3TransformCoord(lPosition, lWorld);
	//			XMStoreFloat3(&position, lPosition);
	//		}
	//	}

	//	XMFLOAT3 targets[6] =
	//	{
	//		XMFLOAT3(position.x + 1.0f, position.y, position.z),
	//		XMFLOAT3(position.x - 1.0f, position.y, position.z),
	//		XMFLOAT3(position.x, position.y + 1.0f, position.z),
	//		XMFLOAT3(position.x, position.y - 1.0f, position.z),
	//		XMFLOAT3(position.x, position.y, position.z + 1.0f),
	//		XMFLOAT3(position.x, position.y, position.z - 1.0f)
	//	};

	//	XMFLOAT3 ups[6] =
	//	{
	//		XMFLOAT3(0.0f, 1.0f, 0.0f),
	//		XMFLOAT3(0.0f, 1.0f, 0.0f),
	//		XMFLOAT3(0.0f, 0.0f, -1.0f),
	//		XMFLOAT3(0.0f, 0.0f, 1.0f),
	//		XMFLOAT3(0.0f, 1.0f, 0.0f),
	//		XMFLOAT3(0.0f, 1.0f, 0.0f)
	//	};

	//	float angle = 90 * XM_PI / 180.f; // we need 90 gradus angle
	//	float aspect = 1.0f; // because out cube map texture is square
	//	for (int i = 0; i < PASSCONSTBUF_COUNT_DCM; i++)
	//	{
	//		m_camerasDCM[i].lookAt(position, targets[i], ups[i]);
	//		m_camerasDCM[i].lens = new PerspectiveCameraLens();

	//		m_camerasDCM[i].lens->setLens(angle, aspect, 1.0f, 100.0f);
	//		m_camerasDCM[i].updateViewMatrix();
	//	}
	//}

	ApplLogger::getLogger().log("TechDemo::init3D()::before Cmd list execution.", 0);
	m_cmdList->Close();
	ID3D12CommandList* cmdsList[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdsList);
	ApplLogger::getLogger().log("TechDemo::init3D()::Cmd list execution is done.", 0);

	FlushCommandQueue();
	ApplLogger::getLogger().log("TechDemo::init3D()::Flush commands is done.", 0);
	m_init3D_done = true;

	build_OffsetVectors();

	m_animationTimer.tt_RunStop();

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
			float lTickTime = m_animationTimer.deltaTime()/10.0f;

			SkinnedData& lSkeleton = m_skeletonManager.getSkeletonNodeAnimated(0); /*TO_DO: now we use the first Skeleton. fix to get Skeleton by ID from Camera object*/
			XMFLOAT4X4 &lFinalMatrices = lSkeleton.getNodeTransform(lTickTime, 0);			

			m_camera->transform(lFinalMatrices);
		}
	}
}

void TechDemo::update_objectCB()
{	
	if (!m_scene.isInstancesDataUpdateRequred()) return;
	
	UINT lInstancesCount = 0;
	auto lInstances = m_scene.getInstancesUpdate(lInstancesCount);
	std::vector<UINT>& lDrawInstancesID = m_scene.getDrawInstancesID(); //TO_DO: Delete DrawInstancesID technique

	if (lInstancesCount == 0) return;

	auto currCBObject = m_frameResourceManager.currentFR()->getObjectCB();
	auto currDrawCBObject = m_frameResourceManager.currentFR()->getDrawInstancesCB();

	for (int i=0; i< lInstancesCount; i++)
		currCBObject->CopyData(i, *lInstances[i]);		

	currDrawCBObject->CopyData(0, lDrawInstancesID.size(), lDrawInstancesID.data());

	m_tempVal++;
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

	const std::vector<CPULight>& lights = m_scene.getLights();

	// -- Find shadow box position and sise	
	float lLenght = 38.0f;

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
	It works good when we look mainly forward, not down from high distance*/

	XMStoreFloat3(&lcameraPos, lCameraPosv);	

	for (size_t i = 0; i < lights.size(); i++)
	{
		mMainPassCB.Lights[i].Direction = lights.at(i).Direction;
		mMainPassCB.Lights[i].Strength = lights.at(i).Strength;
		mMainPassCB.Lights[i].Position = lights.at(i).Position;
		mMainPassCB.Lights[i].spotPower = lights.at(i).spotPower;
		mMainPassCB.Lights[i].falloffStart = lights.at(i).falloffStart;
		mMainPassCB.Lights[i].falloffEnd = lights.at(i).falloffEnd;
		mMainPassCB.Lights[i].lightType = lights.at(i).lightType + 1; // 0 - is undefined type of light
		mMainPassCB.Lights[i].turnOn = lights.at(i).turnOn;	

		// build ViewProjT matrix for shadow technique
		if (lights.at(i).lightType == LightType::Directional)
			MathHelper::buildSunOrthoLightProjection(mMainPassCB.Lights[i].Direction, mMainPassCB.Lights[i].ViewProj,
				mMainPassCB.Lights[i].ViewProjT, lcameraPos, lLenght);
	}
	
	auto currPassCB = m_frameResourceManager.currentFR()->getPassCB();
	currPassCB->CopyData(PASSCONSTBUF_ID_MAIN, mMainPassCB);

	
	//// Update Pass CB for Dynamic Cube Map drawcalls
	//if (PASSCONSTBUF_COUNT_DCM > 0)
	//{
	//	for (int i = 0; i < PASSCONSTBUF_COUNT_DCM; i++)
	//	{
	//		XMMATRIX view = m_camerasDCM[i].getView();
	//		XMMATRIX proj = m_camerasDCM[i].lens->getProj();

	//		XMMATRIX viewProj = XMMatrixMultiply(view, proj);			
	//		XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);			
	//		mMainPassCB.EyePosW = m_camerasDCM[i].getPosition3f();

	//		XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));			
	//		XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	//		XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	//		XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));			

	//		currPassCB->CopyData(PASSCONSTBUF_ID_DCM + i, mMainPassCB);
	//	}
	//}	

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
	// Calculate koeffs for ComputeRender

	float c = 4.0f;
	float t = 0.029f;
	float dx = 0.058f;// 3921568f;
	float mu = 0.2f;

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

void TechDemo::build_defaultCamera()
{
	m_camera = new Camera();
	//DirectX::XMVECTOR pos = DirectX::XMVectorSet(-1.0, 3.0f, -13.0f, 1.0f);
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(4.0, 5.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorSet(0.0, 0.0f, 0.0f, 1.0f);// DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_camera->lookAt(pos, target, up);

	float w = static_cast<float>(width());
	float h = static_cast<float>(height());
	float aspect = w / h;
	m_camera->lens->setLens(0.25f*XM_PI, aspect, 1.0f, 100.0f);

	m_camera->buildFrustumBounding();

	m_defaultCamera = true;
}

void TechDemo::work()
{
	FlushCommandQueue();
	// here we begin new Graphic Frame
	m_frameResourceManager.getFreeFR(); // so we need new Frame resource

	// for graphic frame we use CommandAllocator from FR
	m_frameResourceManager.changeCmdAllocator(m_cmdList.Get(), nullptr);
	
	// update data
	update();

	// draw data
	m_renderManager.draw();

	m_cmdList->Close();
	ID3D12CommandList* CmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, CmdLists);

#ifdef GUI_HUD
	renderUI();
#endif

	m_swapChain->Present(0, 0);
	m_frameResourceManager.currentFR()->setFenceValue(getFenceValue());
	//setFence(); ???
	FlushCommandQueue();
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
	m_HUDContext->DrawTextW(
		wtest.c_str(),
		wtest.length(),
		m_textFormat.Get(),
		ltextRect,
		m_HUDBrush.Get());

	HRESULT res = m_HUDContext->EndDraw();
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
		m_camera->buildFrustumBounding();
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