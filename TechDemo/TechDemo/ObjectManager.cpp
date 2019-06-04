#include "ObjectManager.h"

using namespace std;

ObjectManager::ObjectManager()
{
}

ObjectManager::~ObjectManager()
{		
	
}


void ObjectManager::addOpaqueObject(unique_ptr<RenderItem>& p)
{
	m_opaqueLayer.push_back(std::move(p));	
}

void ObjectManager::addTransparentObject(unique_ptr<RenderItem>& p)
{
	assert(0);
}
void ObjectManager::addAnimatedObject(unique_ptr<RenderItem>& p)
{
	assert(0);
}

const vector<unique_ptr<RenderItem>>& ObjectManager::getOpaqueLayer()
{
	return m_opaqueLayer;
}

void ObjectManager::addMesh(std::string name, std::unique_ptr<Mesh>& iMesh)
{
	auto it = m_meshes.find(name);
	assert(it == m_meshes.end());// we do not expect more meshes with the same name
	m_meshes[name] = std::move(iMesh);
}