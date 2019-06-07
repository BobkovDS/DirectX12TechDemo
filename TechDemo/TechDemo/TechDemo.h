#pragma once
#include "BasicDXGI.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "ResourceManager.h"
#include "Utilit3D.h"
#include "FBXFileLoader.h"
#include "RenderManager.h"

class TechDemo :
	public BasicDXGI
{
	ObjectManager m_objectManager;
	Scene m_scene;
	ResourceManager m_resourceManager;
	RenderManager m_renderManager;
	FrameResourcesManager<InstanceDataGPU, PassConstantsGPU, SSAO_GPU> m_frameResourceManager;
	Utilit3D m_utilit3D;
	FBXFileLoader m_fbx_loader;
	
protected:
	void init3D();
	void update();
	
public:
	TechDemo(HINSTANCE hInstance, const std::wstring& applName, int width, int height);
	~TechDemo();

};

