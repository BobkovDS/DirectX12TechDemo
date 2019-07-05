#include "TechDemo.h"
#include "ApplLogger.h"

#define PASSCONSTBUFCOUNTDCM 6
#define PASSCONSTBUFCOUNT 1/*Frame*/ + 0/*Shadow*/ +0/*Mirror*/ + PASSCONSTBUFCOUNTDCM
#define SSAOCONSTBUFCOUNT 1

using namespace DirectX;

TechDemo::TechDemo(HINSTANCE hInstance, const std::wstring& applName, int width, int height)
	:BasicDXGI(hInstance, applName, width, height)	
{
	m_init3D_done = false;
	m_isTechFlag = false;
	m_isCameraManualControl = true;
	//Utilit3D::nullitizator();
}
TechDemo::~TechDemo()
{
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
		m_isTechFlag = !m_isTechFlag;
		m_animationTimer.tt_Pause();
	}
	case 'C': m_isCameraManualControl = !m_isCameraManualControl; break;
	case VK_NUMPAD0: m_renderManager.toggleDebugMode(); break;
	case VK_NUMPAD1: m_renderManager.toggleDebug_Axes(); break;
	case VK_NUMPAD2: m_renderManager.toggleDebug_Lights(); break;
	case VK_NUMPAD3: m_renderManager.toggleDebug_Normals_Vertex(); break;
	case '1': m_renderManager.setRenderMode_Final(); break;
	case '2': m_renderManager.setRenderMode_SSAO_Map1(); break;
	case '3': m_renderManager.setRenderMode_SSAO_Map2(); break;
	case '4': m_renderManager.setRenderMode_SSAO_Map3(); break;
	default:
		break;
	}
}

std::string TechDemo::addTextToWindow()
{
	int lInstanceCount = m_scene.getInstances().size();
	std::string text = " InstCount: " + std::to_string(lInstanceCount);
	
	if (m_isTechFlag)
		text += " TechFlag=1";
	else
		text += " TechFlag=0";

	if (m_isCameraManualControl)
		text += " CameraMC=1";
	else
		text += " CameraMC=0";

	if (m_renderManager.isDebugMode())
		text += " DEBUG ";

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

	////Load a house
	{
		FBXFileLoader m_fbx_loader;
		m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
		m_fbx_loader.loadSceneFile("Models\\The Scene.fbx");		
	}	

	//// Load a Tree
	//{
	//	FBXFileLoader m_fbx_loader;
	//	m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
	//	m_fbx_loader.loadSceneFile("Models\\Tree1.fbx");
	//}
 
	m_tempVal = 0;
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
		PASSCONSTBUFCOUNT, SSAOCONSTBUFCOUNT, lBonesCount);
	
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

	ExecuterVoidVoid<Scene>* sceneCameraListener =
		new ExecuterVoidVoid<Scene>(&m_scene, &Scene::cameraListener);
	m_camera->addObserver(sceneCameraListener);

	//m_objectManager.mirrorZ();

	m_scene.build(&m_objectManager, m_camera, &m_skeletonManager);
	m_renderManager.initialize(lRenderManagerParams);
	m_renderManager.buildRenders();	
	

	m_cmdList->Close();
	ID3D12CommandList* cmdsList[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdsList);

	FlushCommandQueue();
	m_init3D_done = true;

	build_OffsetVectors();

	m_animationTimer.tt_RunStop();
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
	
	auto lInstances = m_scene.getInstancesUpdate();

	if (lInstances.size() == 0) return;

	auto currCBObject = m_frameResourceManager.currentFR()->getObjectCB();

	InstanceDataGPU lInst; /*TO_DO: delete*/
	lInst = *lInstances[0];

	for (int i=0; i< lInstances.size(); i++)
		currCBObject->CopyData(i, *lInstances[i]);	
	m_tempVal++;
}

void TechDemo::update_BoneData()
{	
	float lTickTime = m_animationTimer.deltaTime();

	//m_animTime = 0;
	auto currBoneCB = m_frameResourceManager.currentFR()->getBoneCB();
	float lBoneCBID= 0;

	int lSkeletonCount = m_skeletonManager.getSkeletonSkinnedAnimatedCount();
	for (int si = 0; si < lSkeletonCount; si++)
	{
		SkinnedData& lSkeleton = m_skeletonManager.getSkeletonSkinnedAnimated(si);
		const std::vector<DirectX::XMFLOAT4X4>& lFinalMatrices = lSkeleton.getFinalTransforms(lTickTime, 0);

		InstanceDataGPU ltmp;
		for (int i = 0; i < lFinalMatrices.size(); i++)
		{
			currBoneCB->CopyData(lBoneCBID++, lFinalMatrices[i]);
			//currBoneCB->CopyData(lBoneCBID++, ltmp.World);
		}
	}
}

void TechDemo::update_passCB()
{
	auto mMainPassCB = m_frameResourceManager.tmpPassConsts;

	XMMATRIX view = m_camera->getView();
	XMMATRIX proj = m_camera->lens->getProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	//XMStoreFloat4x4(&mMainPassCB.ReflectWord, XMMatrixTranspose(perpectViewProj)); // Reflection here is used for Projector Proj matrix

	mMainPassCB.RenderTargetSize = XMFLOAT2((float)width(), (float)height());
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / width(), 1.0f / height());
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.EyePosW = m_camera->getPosition3f();
	mMainPassCB.TotalTime = m_animationTimer.totalTime();
	mMainPassCB.doesUseMirrorLight = 0; // should we use Mirror light?

	const std::vector<CPULight>& lights = m_scene.getLights();

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
	}
	
	auto currPassCB = m_frameResourceManager.currentFR()->getPassCB();
	currPassCB->CopyData(0, mMainPassCB);	
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

	XMStoreFloat4x4(&mSSAOPassCB.ProjTex, XMMatrixTranspose(textProj));
	std::copy(&m_offsets[0], &m_offsets[14], &mSSAOPassCB.OffsetVectors[0]);

	mSSAOPassCB.OcclusionRadius = 1.0f;
	mSSAOPassCB.OcclusionFadeStart = 0.2;
	mSSAOPassCB.OcclusionFadeEnd = 1.0f;
	mSSAOPassCB.SurfaceEpsilon = 0.05f;

	auto blurWeights = calcGaussWeights(2.5f);
	mSSAOPassCB.BlurWeight[0] = XMFLOAT4(&blurWeights[0]);
	mSSAOPassCB.BlurWeight[1] = XMFLOAT4(&blurWeights[4]);
	mSSAOPassCB.BlurWeight[2] = XMFLOAT4(&blurWeights[8]);

	mSSAOPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / width(), 1.0f / height());
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
	// here we begin new Graphic Frame
	m_frameResourceManager.getFreeFR(); // so we need new Frame resource

	// for graphic frame we use CommandAllocator from FR
	m_frameResourceManager.changeCmdAllocator(m_cmdList.Get(), nullptr);

	// update data
	update();

	// draw data
	int lTechFlag;
	
	if (m_isTechFlag)
		lTechFlag |= 0x01;
	else
		lTechFlag &= ~0x01;

	m_renderManager.draw(lTechFlag);


	m_cmdList->Close();
	ID3D12CommandList* CmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, CmdLists);

	m_swapChain->Present(0, 0);
	m_frameResourceManager.currentFR()->setFenceValue(getFenceValue());
	setFence();
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