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

class TechDemo :
	public BasicDXGI
{
	ObjectManager m_objectManager;
	Scene m_scene;
	ResourceManager m_resourceManager;
	RenderManager m_renderManager;
	SkeletonManager m_skeletonManager;
	FrameResourcesManager<InstanceDataGPU, PassConstantsGPU, SSAO_GPU> m_frameResourceManager;
	Utilit3D m_utilit3D;
	Camera* m_camera;
	Timer m_animationTimer;

	bool m_init3D_done;
	bool m_defaultCamera;
	bool m_mouseDown;
	bool m_isFog;
	bool m_isTechFlag;
	bool m_isCameraManualControl;
	POINT m_mouseDownPoint;
	float m_animTime;
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
	void update_objectCB();
	void build_defaultCamera();
	void work();
	void onReSize(int newWidth, int newHeight);
public:
	TechDemo(HINSTANCE hInstance, const std::wstring& applName, int width, int height);
	~TechDemo();

};

