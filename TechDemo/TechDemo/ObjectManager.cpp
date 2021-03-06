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

void ObjectManager::addTransparentObjectGH(unique_ptr<RenderItem>& p)
{
	m_notOpaqueLayerGH.push_back(std::move(p));
}

void ObjectManager::addTransparentObjectCH(unique_ptr<RenderItem>& p)
{
	m_notOpaqueLayerCH.push_back(std::move(p));
}

void ObjectManager::addSkinnedOpaqueObject(unique_ptr<RenderItem>& p)
{
	m_skinnedOpaqueLayer.push_back(std::move(p));
}

void ObjectManager::addSkinnedNotOpaqueObject(unique_ptr<RenderItem>& p)
{
	m_skinnedNotOpaqueLayer.push_back(std::move(p));
}

void ObjectManager::addSky(std::unique_ptr<RenderItem>& p)
{
	m_skyLayer.push_back(std::move(p));
}

void ObjectManager::addMesh(std::string name, std::unique_ptr<Mesh>& iMesh)
{
	auto it = m_meshes.find(name);
	assert(it == m_meshes.end());// we do not expect more meshes with the same name
	m_meshes[name] = std::move(iMesh);
}

void ObjectManager::addCamera(std::unique_ptr<Camera>& camera)
{
	m_cameras.push_back(std::move(camera));
}

void ObjectManager::addLight(LightCPU light)
{
	m_lights.push_back(light);
}

vector<LightCPU>& ObjectManager::getLights()
{
	return m_lights;
}

const vector<std::unique_ptr<Camera>>& ObjectManager::getCameras()
{
	return m_cameras;
}

const vector<unique_ptr<RenderItem>>& ObjectManager::getSky()
{
	return m_skyLayer;
}

const vector<unique_ptr<RenderItem>>& ObjectManager::getOpaqueLayer()
{
	return m_opaqueLayer;
}

const vector<unique_ptr<RenderItem>>& ObjectManager::getNotOpaqueLayer()
{
	return m_notOpaqueLayer;
}

const vector<unique_ptr<RenderItem>>& ObjectManager::getNotOpaqueLayerGH()
{
	return m_notOpaqueLayerGH;
}

const vector<unique_ptr<RenderItem>>& ObjectManager::getNotOpaqueLayerCH()
{
	return m_notOpaqueLayerCH;
}

const vector<unique_ptr<RenderItem>>& ObjectManager::getSkinnedOpaqueLayer()
{
	return m_skinnedOpaqueLayer;
}

const vector<unique_ptr<RenderItem>>& ObjectManager::getSkinnedNotOpaqueLayer()
{
	return m_skinnedNotOpaqueLayer;
}

UINT ObjectManager::getCommonInstancesCount()
{
	UINT lCommonInstancesCount = 0;

	for (int ri = 0; ri < m_skyLayer.size(); ri++)
		lCommonInstancesCount += m_skyLayer[ri]->Instances.size();

	for (int ri = 0; ri < m_opaqueLayer.size(); ri++)
		lCommonInstancesCount += m_opaqueLayer[ri]->Instances.size();

	for (int ri = 0; ri < m_notOpaqueLayer.size(); ri++)
		lCommonInstancesCount += m_notOpaqueLayer[ri]->Instances.size();

	for (int ri = 0; ri < m_skinnedOpaqueLayer.size(); ri++)
		lCommonInstancesCount += m_skinnedOpaqueLayer[ri]->Instances.size();
	
	for (int ri = 0; ri < m_skinnedNotOpaqueLayer.size(); ri++)
		lCommonInstancesCount += m_skinnedNotOpaqueLayer[ri]->Instances.size();
	
	for (int ri = 0; ri < m_notOpaqueLayerGH.size(); ri++)
		lCommonInstancesCount += m_notOpaqueLayerGH[ri]->Instances.size();

	return lCommonInstancesCount;
}

