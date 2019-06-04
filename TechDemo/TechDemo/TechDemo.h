#pragma once
#include "BasicDXGI.h"
#include "ObjectManager.h"
#include "ResourceManager.h"
#include "Utilit3D.h"
#include "FBXFileLoader.h"
class TechDemo :
	public BasicDXGI
{
	ObjectManager m_objectManager;
	ResourceManager m_resourceManager;
	Utilit3D m_utilit3D;
	FBXFileLoader m_fbx_loader;
protected:
	void init3D();
	void update();
	
public:
	TechDemo(HINSTANCE hInstance, const std::wstring& applName, int width, int height);
	~TechDemo();

};

