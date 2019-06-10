#include "Scene.h"

Scene::Scene()
{
	m_Layers.resize(4);
	m_doesItNeedUpdate = true;
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

void Scene::build(ObjectManager* objectManager)
{
	assert(objectManager);
	m_objectManager = objectManager;

	m_Layers[0].setVisibility(true);
	m_Layers[1].setVisibility(true);
	m_Layers[2].setVisibility(true);
	m_Layers[3].setVisibility(true);

	update();
}

void Scene::update()
{
	if (!m_doesItNeedUpdate) return;
	
	updateLayer(m_Layers[0], m_objectManager->getOpaqueLayer());
	updateLayer(m_Layers[1], m_objectManager->getNotOpaqueLayer());
	updateLayer(m_Layers[2], m_objectManager->getSkinnedOpaqueLayer());
	updateLayer(m_Layers[3], m_objectManager->getSkinnedNotOpaqueLayer());	
	
	m_doesItNeedUpdate = false;
}

void Scene::updateLayer(SceneLayer& layer, const std::vector<std::unique_ptr<RenderItem>>& arrayRI)
{
	layer.clearLayer();

	for (int i = 0; i < arrayRI.size(); i++)
	{
		SceneLayer::SceneLayerObject lSceneObject = {};

		RenderItem* lRI = arrayRI[i].get();
		lSceneObject.setObjectMesh(lRI);
		for (int j = 0; j < lRI->Instances.size(); j++)
			lSceneObject.addInstance(&lRI->Instances[j]);

		layer.addSceneObject(lSceneObject);
	}
}
// ================================================================ [Scene::SceneLayer] =============================== 

void Scene::SceneLayer::clearLayer()
{
	m_objects.clear();
}

bool Scene::SceneLayer::isLayerVisible()
{
	return m_visible;
}

void Scene::SceneLayer::setVisibility(bool b)
{
	m_visible = b;
}

int Scene::SceneLayer::getLayerInstancesCount()
{
	return 0;// test
}

int Scene::SceneLayer::getSceneObjectCount()
{
	return m_objects.size();
}

void Scene::SceneLayer::addSceneObject(SceneLayerObject sceneObject)
{
	m_objects.push_back(sceneObject);
}

Scene::SceneLayer::SceneLayerObject* Scene::SceneLayer::getSceneObject(UINT sceneObjectIndex)
{
	if (sceneObjectIndex < m_objects.size())
		return &m_objects[sceneObjectIndex];
	else
		return nullptr;
}
// ================================================================ [Scene::SceneLayer::SceneLayerObject] =============
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



