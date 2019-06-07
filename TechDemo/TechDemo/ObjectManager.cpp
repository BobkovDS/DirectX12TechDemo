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
	m_notOpaqueLayer.push_back(std::move(p));
}

void ObjectManager::addSkinnedOpaqueObject(unique_ptr<RenderItem>& p)
{
	m_skinnedOpaqueLayer.push_back(std::move(p));
}

void ObjectManager::addSkinnedNotOpaqueObject(unique_ptr<RenderItem>& p)
{
	m_skinnedNotOpaqueLayer.push_back(std::move(p));
}

const vector<unique_ptr<RenderItem>>& ObjectManager::getOpaqueLayer()
{
	return m_opaqueLayer;
}

const vector<unique_ptr<RenderItem>>& ObjectManager::getNotOpaqueLayer()
{
	return m_notOpaqueLayer;
}

const vector<unique_ptr<RenderItem>>& ObjectManager::getSkinnedOpaqueLayer()
{
	return m_skinnedOpaqueLayer;
}

const vector<unique_ptr<RenderItem>>& ObjectManager::getSkinnedNotOpaqueLayer()
{
	return m_skinnedNotOpaqueLayer;
}

void ObjectManager::addMesh(std::string name, std::unique_ptr<Mesh>& iMesh)
{
	auto it = m_meshes.find(name);
	assert(it == m_meshes.end());// we do not expect more meshes with the same name
	m_meshes[name] = std::move(iMesh);
}