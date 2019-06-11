#pragma once
#include "Utilit3D.h"
#include "BasicDXGI.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "ResourceManager.h"
#include "SkinnedData.h"
#include "FBXFileLoader.h"
#include "RenderManager.h"

class TechDemo :
	public BasicDXGI
{
	ObjectManager m_objectManager;
	Scene m_scene;
	ResourceManager m_resourceManager;
	RenderManager m_renderManager;
	SkinnedData m_skinnedData;
	FrameResourcesManager<InstanceDataGPU, PassConstantsGPU, SSAO_GPU> m_frameResourceManager;
	Utilit3D m_utilit3D;
	FBXFileLoader m_fbx_loader;
	
protected:
	void init3D();
	void update();
	void update_BoneData();
	void work();
	
public:
	TechDemo(HINSTANCE hInstance, const std::wstring& applName, int width, int height);
	~TechDemo();

};

