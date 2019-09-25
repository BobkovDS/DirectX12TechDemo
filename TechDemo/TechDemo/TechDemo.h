#pragma once
#include "Utilit3D.h"
#include "BasicDXGI.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "ResourceManager.h"
#include "SkeletonManager.h"
#include "FBXFileLoader.h"
#include "RenderManager.h"
#include "Camera.h"
#include "LogoRender.h"
#include <Thread>

// How many Pass constant buffers we have
#define PASSCONSTBUF_COUNT_MAIN 1
#define PASSCONSTBUF_COUNT_MIRROR 1
#define PASSCONSTBUF_COUNT_DCM 6
#define PASSCONSTBUFCOUNT PASSCONSTBUF_COUNT_MAIN + PASSCONSTBUF_COUNT_MIRROR + PASSCONSTBUF_COUNT_DCM 

// ID for this Pass constant buffers
#define PASSCONSTBUF_ID_MAIN 0
#define PASSCONSTBUF_ID_MIRROR PASSCONSTBUF_ID_MAIN + PASSCONSTBUF_COUNT_MAIN
#define PASSCONSTBUF_ID_DCM PASSCONSTBUF_ID_MIRROR + PASSCONSTBUF_COUNT_MIRROR

// SSAO Pass constant buffers count
#define SSAOCONSTBUFCOUNT 1

class TechDemo :
	public BasicDXGI
{
	static const int MaxBlurRadius = 5;
	static const int MaxInstancesCount = 1000000;
	
	ObjectManager m_objectManager;
	Scene m_scene;
	ResourceManager m_resourceManager;
	RenderManager m_renderManager;
	SkeletonManager m_skeletonManager;	
	FrameResourcesManager<InstanceDataGPU, PassConstantsGPU, SSAO_GPU> m_frameResourceManager;	
	Utilit3D m_utilit3D;
	Camera* m_camera;
	Camera m_camerasDCM[PASSCONSTBUF_COUNT_DCM]; // Cameras for DCM (Dynamic Cube Map buidling)
	Timer m_animationTimer;
	Timer m_framePrepareAndDrawTimer;
	Timer m_betweenFramesTimer;
	std::thread m_logoThread;
	DirectX::XMFLOAT4 m_offsets[14];
	UINT* m_drawInstancesIDs;

	bool m_init3D_done;
	bool m_defaultCamera;
	bool m_mouseDown;
	bool m_isFog;
	bool m_isTechFlag;
	bool m_isCameraManualControl;
	POINT m_mouseDownPoint;
	float m_animTime;
	float m_framePreparationTime;
	float m_betweenFramesTime;

	int m_tempVal;

	void onMouseDown(WPARAM btnState, int x, int y);
	void onMouseUp(WPARAM btnState, int x, int y);
	void onMouseMove(WPARAM btnState, int x, int y);
	void onKeyDown(WPARAM btnState);
	std::string addTextToWindow();
protected:
	void init3D();
	void update();
	void update_BoneData();
	void update_camera();
	void update_light();
	void update_passCB();
	void update_passMirror(PassConstantsGPU& passCB);
	void update_passSSAOCB();
	void update_objectCB();
	void build_defaultCamera();
	void build_OffsetVectors();
	void work();
	void onReSize(int newWidth, int newHeight);
	std::vector<float> calcGaussWeights(float sigma);
	void renderUI();
public:
	TechDemo(HINSTANCE hInstance, const std::wstring& applName, int width, int height);
	~TechDemo();

};

