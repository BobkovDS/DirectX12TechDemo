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

const std::vector<CPULight>& Scene::getLights()
{
	if (m_lights.size() == 0) //if we do not have any light, lets add default one
	{
		/*directional light.  We should know:
			1) position
			2) strenght
			3) diraction
		*/

		DirectX::XMVECTOR pos, direction;

		pos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		direction = DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);

		CPULight tmpLight = {};
		tmpLight.lightType = LightType::Directional;
		tmpLight.mPhi = 0;
		tmpLight.mTheta = 0;
		tmpLight.mRadius = 0;

		XMStoreFloat3(&tmpLight.Direction, direction);
		XMStoreFloat3(&tmpLight.initDirection, direction);
		XMStoreFloat3(&tmpLight.Position, pos);

		tmpLight.Strength = DirectX::XMFLOAT3(0.8f, 0.8f, 0.8f);

		tmpLight.spotPower = 4;
		tmpLight.falloffStart = 1;
		tmpLight.falloffEnd = tmpLight.mRadius;
		tmpLight.shapeID = -1;

		tmpLight.posRadius = 0;
		tmpLight.posTheta = 0;
		tmpLight.posPhi = 0;

		tmpLight.needToUpdateRI = 1;
		tmpLight.needToUpdateLight = 1;

		m_lights.push_back(tmpLight);
	}

	return m_lights;
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



