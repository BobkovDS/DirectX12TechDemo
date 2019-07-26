#include "Scene.h"

using namespace std;
using namespace DirectX;

UINT Scene::ContainsCount = 0;

Scene::Scene()
{
	m_Layers.resize(6);
	m_doesItNeedUpdate = true;
	m_firstBB = true;
	m_octreeCullingMode = true;
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
		if (m_Layers[i].isLayerVisible())
		offset += m_Layers[i].getLayerInstancesCount();
	return offset;
}

std::vector<const InstanceDataGPU*>& Scene::getInstancesUpdate()
{
	m_tmp_Intances.clear();
	m_tmp_drawInstancesID.clear();

	for (int i = 0; i < m_Layers.size(); i++)	
		if (m_Layers[i].isLayerVisible())
		{
			UINT lInstancesPerLayerCount = m_tmp_Intances.size();
			m_Layers[i].getInstances(m_tmp_Intances, m_tmp_drawInstancesID, lInstancesPerLayerCount);
		}	
	m_instancesDataReadTimes--;

	return m_tmp_Intances;
}

std::vector<const InstanceDataGPU*>& Scene::getInstances()
{
	return m_tmp_Intances;
}

std::vector<UINT>& Scene::getDrawInstancesID()
{
	return m_tmp_drawInstancesID;
}

void Scene::build(ObjectManager* objectManager, Camera* camera, SkeletonManager* skeletonManager)
{
	assert(objectManager);
	assert(camera);
	assert(skeletonManager);
	m_objectManager = objectManager;
	m_camera = camera;
	m_skeletonManager = skeletonManager;

	m_Layers[0].setVisibility(true); // Sky
	m_Layers[1].setVisibility(true); // Simple Opaque
	m_Layers[2].setVisibility(true); // Simple Not Opaque 
	m_Layers[3].setVisibility(true); // Skinned Opaque
	m_Layers[4].setVisibility(false); // Skinned Not Opaque
	m_Layers[5].setVisibility(true); // Tesselated object (water)

	// Copy lights once to Scene		
	for (int i = 0; i < m_objectManager->getLights().size(); i++)
		m_lights.push_back(m_objectManager->getLights()[i]);

	createBoungingInfo();
	buildOctree();
	update();
}

void Scene::createBoungingInfo()
{	
	getLayerBoundingInfo(m_sceneBB, m_sceneBBShadow, m_objectManager->getOpaqueLayer());
	getLayerBoundingInfo(m_sceneBB, m_sceneBBShadow, m_objectManager->getNotOpaqueLayer());
	getLayerBoundingInfo(m_sceneBB, m_sceneBBShadow, m_objectManager->getSkinnedOpaqueLayer());
	getLayerBoundingInfo(m_sceneBB, m_sceneBBShadow, m_objectManager->getSkinnedNotOpaqueLayer());
	//getLayerBoundingInfo(m_sceneBB, m_objectManager->getNotOpaqueLayerGH());

	m_sceneBB.Extents.x *= 1.1f;
	m_sceneBB.Extents.y *= 1.1f;
	m_sceneBB.Extents.z *= 1.1f;

	m_sceneBBShadow.Extents.x *= 1.1f;
	m_sceneBBShadow.Extents.y *= 1.1f;
	m_sceneBBShadow.Extents.z *= 1.1f;
	
	BoundingSphere::CreateFromBoundingBox(m_sceneBS, m_sceneBB); // Create Bounding Sphere for all Scene
	BoundingSphere::CreateFromBoundingBox(m_sceneBSShadow, m_sceneBBShadow);// Create Bounding Sphere for Scene where will used Shadowing (without Land and Water)
}

void Scene::addOctreeInformation(const std::vector<std::unique_ptr<RenderItem>>& arrayRI, vector<BoundingBoxEXT*>& lLayerBBList)
{	
	// get All RenderItems/Instances for current SceneLayer
	for (int ri = 0; ri < arrayRI.size(); ri++)
	{
		for (int i = 0; i < arrayRI[ri]->Instances.size(); i++)
		{
			BoundingBox ltBB;
			XMMATRIX lWorld = XMLoadFloat4x4(&arrayRI[ri]->Instances[i].World);
			arrayRI[ri]->AABB.Transform(ltBB, lWorld);
			BoundingBoxEXT* lBBExt = new BoundingBoxEXT(ltBB);
			lBBExt->pRenderItem = arrayRI[ri].get();
			lBBExt->Inst_ID = i;
			lLayerBBList.push_back(lBBExt);
		}
	}
}

void Scene::buildOctree()
{
	assert(m_octree == nullptr); // it should not be created yet
	
	m_octree = new Octree(m_sceneBB); 

	vector<BoundingBoxEXT*> lLayerBBList;

	addOctreeInformation(m_objectManager->getOpaqueLayer(), lLayerBBList);
	addOctreeInformation(m_objectManager->getNotOpaqueLayer(), lLayerBBList);
	addOctreeInformation(m_objectManager->getSkinnedOpaqueLayer(), lLayerBBList);
	addOctreeInformation(m_objectManager->getSkinnedNotOpaqueLayer(), lLayerBBList);
	addOctreeInformation(m_objectManager->getNotOpaqueLayerGH(), lLayerBBList);

	if (lLayerBBList.size() == 0) return;

	m_octree->addBBList(lLayerBBList);
	m_octree->build();
}

void Scene::update()
{
	if (!m_doesItNeedUpdate) return;
		
	ContainsCount = 0;
	if (m_octreeCullingMode)
	{
		//m_octree->update(m_camera->getFrustomBoundingShadowWorld());
		m_octree->update(m_camera->getFrustomBoundingCameraWorld());
		updateLayer(m_Layers[0], m_objectManager->getSky(), false);
		m_Layers[1].update(m_objectManager->getOpaqueLayer());
		m_Layers[2].update(m_objectManager->getNotOpaqueLayer());
		m_Layers[3].update(m_objectManager->getSkinnedOpaqueLayer());
		m_Layers[4].update(m_objectManager->getSkinnedNotOpaqueLayer());
		m_Layers[5].update(m_objectManager->getNotOpaqueLayerGH());		
	}	
	else
	{
		bool lFrustomCulling = false;
		updateLayer(m_Layers[0], m_objectManager->getSky(), lFrustomCulling);
		updateLayer(m_Layers[1], m_objectManager->getOpaqueLayer(), lFrustomCulling);
		updateLayer(m_Layers[2], m_objectManager->getNotOpaqueLayer(), lFrustomCulling);
		updateLayer(m_Layers[3], m_objectManager->getSkinnedOpaqueLayer(), lFrustomCulling);
		updateLayer(m_Layers[4], m_objectManager->getSkinnedNotOpaqueLayer(), lFrustomCulling);
		updateLayer(m_Layers[5], m_objectManager->getNotOpaqueLayerGH(), lFrustomCulling);
	}

	m_instancesDataReadTimes = FRAMERESOURCECOUNT;
	m_doesItNeedUpdate = false;	
}

void Scene::updateLight(float time)
{
	if (!m_lightAnimation) return;

	for (int i = 0; i < m_lights.size(); i++)
	{
		if (m_skeletonManager->isExistSkeletonNodeAnimated(m_lights[i].Name))
		{
			SkinnedData& lSkeleton = m_skeletonManager->getSkeletonNodeAnimated(m_lights[i].Name);

			XMFLOAT4X4 &lFinalMatrix = lSkeleton.getNodeTransform(time, 0);
			XMMATRIX lLcTr = DirectX::XMLoadFloat4x4(&lFinalMatrix);
			if (XMMatrixIsIdentity(lLcTr)) continue;

			DirectX::XMVECTOR lPos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			DirectX::XMVECTOR lLook = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
			
			lPos = XMVector3Transform(lPos, lLcTr);
			lLook = XMVector3Normalize(XMVector3TransformNormal(lLook, lLcTr));

			XMStoreFloat3(&m_lights[i].Position, lPos);
			XMStoreFloat3(&m_lights[i].Direction, lLook);
			XMStoreFloat3(&m_lights[i].initDirection, lLook);
		}
	}
}

bool Scene::isInstancesDataUpdateRequred()
{
	return m_instancesDataReadTimes > 0;
}

void Scene::cameraListener()
{
	m_doesItNeedUpdate = true;
}

void Scene::updateLayer(SceneLayer& layer, const std::vector<std::unique_ptr<RenderItem>>& arrayRI, bool isFrustumCullingRequired)
{	
	if (!layer.isLayerVisible()) return; // no need to fill this layer if it is not visible

	layer.clearLayer();
	BoundingFrustum& lBoundingFrustomCamera = m_camera->getFrustomBoundingCameraWorld();
	BoundingFrustum& lBoundingFrustomShadowProjector = m_camera->getFrustomBoundingShadowWorld();

	UINT lDrawInstanceID = 0; /*each Layer has his own through-ID for all LayerObjects */
	for (int i = 0; i < arrayRI.size(); i++)
	{
		SceneLayer::SceneLayerObject lSceneObject = {};	
		RenderItem* lRI = arrayRI[i].get();
		lSceneObject.setObjectMesh(lRI);
		lSceneObject.setMaxSizeForDrawingIntancesArray(lRI->Instances.size());
		for (int j = 0; j < lRI->Instances.size(); j++)
		{	
			if (isFrustumCullingRequired)
			{
				XMMATRIX lInstanceWord = XMLoadFloat4x4(&lRI->Instances[j].World);
				
				BoundingBox lRIBBWord;
				lRI->AABB.Transform(lRIBBWord, lInstanceWord);
					
				/*XMMATRIX lInstanceWordInv = XMMatrixInverse(&XMMatrixDeterminant(lInstanceWord), lInstanceWord);				
				BoundingFrustum lLocalSpaceFrustom;
				lBoundingFrustom.Transform(lLocalSpaceFrustom, lInstanceWordInv);
				if (lLocalSpaceFrustom.Contains(lRI->AABB) != DirectX::DISJOINT)*/			

				// If Instance can be seen in Camera, no need to check for Shadow Projector - it is also will be there
				ContainsCount++;
				bool doubleContainsCheck = true;
				if (lBoundingFrustomCamera.Contains(lRIBBWord) != DirectX::DISJOINT)
				{
					lSceneObject.addInstance(&lRI->Instances[j]); // add Instance 
					lSceneObject.setDrawInstanceID(lDrawInstanceID++); //.. and also add information about DrawInstance ID
					doubleContainsCheck = false;
				}
				// But if Instance out of Camera Frustom, it can be still inside of Shadow Prjector Frustum 
				else 					
				if (lBoundingFrustomShadowProjector.Contains(lRIBBWord) != DirectX::DISJOINT)
				{
					lSceneObject.addInstance(&lRI->Instances[j]); // only add Instance
					//lDrawInstanceID++; // no add, but counting next DrawInstance ID
					lSceneObject.setDrawInstanceID(lDrawInstanceID++);
				}

				//if (doubleContainsCheck) ContainsCount++;
			}
			else
			{
				lSceneObject.addInstance(&lRI->Instances[j]);
				lSceneObject.setDrawInstanceID(lDrawInstanceID++);
			}
		}

		lSceneObject.setMinSizeForDrawingIntancesArray();
		layer.addSceneObject(lSceneObject);
	}
}

void Scene::getLayerBoundingInfo(DirectX::BoundingBox& layerBB, DirectX::BoundingBox& layerBBShadow,
	const std::vector<std::unique_ptr<RenderItem>>& arrayRI)
{
	for (int i = 0; i < arrayRI.size(); i++)
	{
		RenderItem* lRI = arrayRI[i].get();

		if (!lRI->isNotIncludeInWorldBB) // isNotIncludeInWorldBB ==true : Do not include this RI's Instances to Shadow SceneBB
		{
			for (int j = 0; j < lRI->Instances.size(); j++)
			{
				XMMATRIX lInstanceWord = XMLoadFloat4x4(&lRI->Instances[j].World);
				XMMATRIX lInstanceWordInv = XMMatrixInverse(&XMMatrixDeterminant(lInstanceWord), lInstanceWord);
				BoundingBox lInstanceBB;
				lRI->AABB.Transform(lInstanceBB, lInstanceWord);

				if (!m_firstBB)
				{
					BoundingBox::CreateMerged(layerBB, layerBB, lInstanceBB);
					BoundingBox::CreateMerged(layerBBShadow, layerBBShadow, lInstanceBB);
				}
				else
				{
					BoundingBox::CreateMerged(layerBB, lInstanceBB, lInstanceBB);
					BoundingBox::CreateMerged(layerBBShadow, lInstanceBB, lInstanceBB);
					m_firstBB = false;
				}
			}
		}		
	}
}

const std::vector<CPULight>& Scene::getLights()
{
	if (m_lights.size() == 0) //if we do not have any light, lets add default one
	{
		/*directional light.  We should know:
			1) position
			2) strenght
			3) direction
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

void Scene::toggleLightAnimation()
{
	m_lightAnimation = !m_lightAnimation;
}

void Scene::toggleCullingMode()
{
	m_octreeCullingMode = !m_octreeCullingMode;
}

// ================================================================ [Scene::SceneLayer] =============================== 

//Scene::SceneLayer::SceneLayerObject::SceneLayerObject():m_drawInstanceIDCount(0)
//{	
//}

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

void Scene::SceneLayer::getInstances(std::vector<const InstanceDataGPU*>& out_Instances, 
	std::vector<UINT>& out_DrawInstancesID, UINT InstancesPerPrevLayer)
{
	for (int i = 0; i < m_objects.size(); i++)
		m_objects[i].getInstances(out_Instances, out_DrawInstancesID, InstancesPerPrevLayer);
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

void Scene::SceneLayer::update(const std::vector<std::unique_ptr<RenderItem>>& arrayRI)
{
	if (!isLayerVisible()) return; // no need to fill this layer if it is not visible

	clearLayer();	

	UINT lDrawInstanceID = 0; /*each Layer has his own through-ID for all LayerObjects */
	for (int ri = 0; ri < arrayRI.size(); ri++)
	{
		SceneLayer::SceneLayerObject lSceneObject = {};
		RenderItem* lRI = arrayRI[ri].get();
		lSceneObject.setObjectMesh(lRI);
		lSceneObject.setMaxSizeForDrawingIntancesArray(lRI->Instances.size());

		/* 
		Note:
			Here we think that we use only one list of Instances information: InstancesID. It is because we use only one Frustom
			for building this list - ShadowFrustom. If we want to use two Frustoms: Camera and Shadow, we should add one more 
			list to RenderItems - DrawInstancesID and add filling of this information in Octree. But SceneLayers and Shaders support it now.
		*/
		for (int i = 0; i < lRI->InstancesID.size(); i++)
		{
			lSceneObject.addInstance(&lRI->Instances[lRI->InstancesID[i]]);
			lSceneObject.setDrawInstanceID(lDrawInstanceID++);
		}

		lRI->InstancesID.clear();
		lSceneObject.setMinSizeForDrawingIntancesArray();
		addSceneObject(lSceneObject);
	}
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

void Scene::SceneLayer::SceneLayerObject::getInstances(std::vector<const InstanceDataGPU*>& out_Instances, 
	std::vector<UINT>& out_DrawInstancesID, UINT InstancesPerPrevLayer)
{
	int lPrevSize = out_Instances.size();
	int lPrevSizeDrawID = out_DrawInstancesID.size();

	out_Instances.resize(lPrevSize + m_instances.size());
	out_DrawInstancesID.resize(lPrevSizeDrawID + m_drawInstancesID.size());

	for (int i = 0; i < m_instances.size(); i++)
		out_Instances[lPrevSize + i] = m_instances[i];	

	for (int i = 0; i < m_drawInstancesID.size(); i++)
		out_DrawInstancesID[lPrevSizeDrawID + i] = m_drawInstancesID[i] + InstancesPerPrevLayer;
}