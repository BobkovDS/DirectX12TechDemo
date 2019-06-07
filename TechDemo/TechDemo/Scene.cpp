#include "Scene.h"

Scene::Scene()
{
	m_Layers.resize(4);
}


Scene::~Scene()
{
}

int Scene::getLayersCount()
{
	return m_Layers.size();
}


Scene::SceneLayer* Scene::getLayer(UINT layerIndex)
{
	if (layerIndex < m_Layers.size())
		return &m_Layers[layerIndex];
	else
		nullptr;
}

int Scene::getLayerInstanceOffset(UINT layerCount)
{
	if (layerCount >= m_Layers.size()) return -1;

	int offset = 0;
	for (int i = 0; i < layerCount; i++)
		offset += m_Layers[i].getLayerInstancesCount();
	return offset;
}

bool Scene::SceneLayer::isLayerVisible()
{
	return m_visible;
}

int Scene::SceneLayer::getLayerInstancesCount()
{
	return 0;// test
}

int Scene::SceneLayer::getSceneObjectCount()
{
	return m_objects.size();
}

Scene::SceneLayer::SceneLayerObject* Scene::SceneLayer::getSceneObject(UINT sceneObjectIndex)
{
	if (sceneObjectIndex < m_objects.size())
		return &m_objects[sceneObjectIndex];
	else
		return nullptr;
}

const RenderItem* Scene::SceneLayer::SceneLayerObject::getObjectMesh()
{
	return m_mesh;
}

void Scene::SceneLayer::SceneLayerObject::setObjectMesh(const RenderItem* objectMesh)
{
	m_mesh = objectMesh;
}

void Scene::SceneLayer::SceneLayerObject::clearInstances()
{
	m_instances.clear();
}

void Scene::SceneLayer::SceneLayerObject::addInstance(const InstanceDataGPU* instance)
{
	m_instances.push_back(instance);
}

std::vector<const InstanceDataGPU*>& Scene::SceneLayer::SceneLayerObject::getInstances()
{
	return m_instances;
}