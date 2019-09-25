#include "Scene.h"

using namespace std;
using namespace DirectX;

UINT Scene::ContainsCount = 0;

Scene::Scene()
{
	m_Layers.resize(7);
	m_doesItNeedUpdate = true;
	m_firstBB = true;
	m_octreeCullingMode = false;	
	m_instancesToDraw = 0;

	m_isAfternoon = true; 
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

std::vector<const InstanceDataGPU*>& Scene::getInstancesUpdate(UINT& instancesCount)
{
	for (int i = 0; i < m_Layers.size(); i++)	
		if (m_Layers[i].isLayerVisible()) // Copy Intances data only for visible layers
		{
			//UINT lInstancesPerLayerCount = m_tmp_Intances.size();
			//m_Layers[i].getInstances(m_tmp_Intances, m_tmp_drawInstancesID, lInstancesPerLayerCount);
			m_Layers[i].getInstances(m_tmp_Intances, instancesCount);
		}	
	m_instancesDataReadTimes--;

	m_instancesToDraw = instancesCount;
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

	m_Layers[SKY].setVisibility(true); // Sky
	m_Layers[OPAQUELAYER].setVisibility(true); // Simple Opaque
	m_Layers[NOTOPAQUELAYER].setVisibility(true); // Simple Not Opaque 
	m_Layers[SKINNEDOPAQUELAYER].setVisibility(true); // Skinned Opaque
	m_Layers[SKINNEDNOTOPAQUELAYER].setVisibility(false); // Skinned Not Opaque
	m_Layers[NOTOPAQUELAYERGH].setVisibility(true); // Tesselated object (water)
	m_Layers[NOTOPAQUELAYERCH].setVisibility(true); // Object colclutated on Compute Shader(WaterV2)


	/* ObjectManager stores Lights in order in which FBXReader put it there (read order), but
	for some application logic we need specific order of Light type, e.g. Directional at first. It because some some Renders
	like SSAO and Shadow renders assume that Directional light in 0-slot. Scene builder takes care about this order.
	*/
	std::vector<LightCPU>& lObjectManagersLigths = m_objectManager->getLights();
	int lLightsCount = lObjectManagersLigths.size();
	for (int lt = 0; lt < LightType::lightCount; lt++)
	{
		for (int i = 0; i < lLightsCount; i++)
		{
			if (lObjectManagersLigths[i].lightType == lt)
				m_lights.push_back(lObjectManagersLigths[i]);
		}
	}
		
	InitLayers();
	buildOctree();
	update();
}

void Scene::buildOctree()
{
	assert(m_octree == nullptr); // it should not be created yet
	
	m_octree = new Octree(m_sceneBB); 	

	if (m_layerBBList.size() == 0) return;

	m_octree->addBBList(m_layerBBList);
	m_octree->build();	
}

void Scene::InitLayers()
{
	m_Layers[SKY].init(m_objectManager->getSky());
	m_Layers[OPAQUELAYER].init(m_objectManager->getOpaqueLayer());
	m_Layers[NOTOPAQUELAYER].init(m_objectManager->getNotOpaqueLayer());
	m_Layers[SKINNEDOPAQUELAYER].init(m_objectManager->getSkinnedOpaqueLayer());
	m_Layers[SKINNEDNOTOPAQUELAYER].init(m_objectManager->getSkinnedNotOpaqueLayer());
	m_Layers[NOTOPAQUELAYERGH].init(m_objectManager->getNotOpaqueLayerGH());
	m_Layers[NOTOPAQUELAYERCH].init(m_objectManager->getNotOpaqueLayerCH());

	UINT lAllInstancesCount = 0;
	for (int i = 0; i < m_Layers.size(); i++)
		lAllInstancesCount += m_Layers[i].getLayerInstancesCount();
	m_tmp_Intances.resize(lAllInstancesCount);

	// make a BoundingBoxExt list for Octree.
	m_Layers[SKY].getBoundingInformation(m_layerBBList, m_layerBBListExcludedFromCulling, nullptr);
	m_Layers[OPAQUELAYER].getBoundingInformation(m_layerBBList, m_layerBBListExcludedFromCulling, &m_sceneBB);
	m_Layers[NOTOPAQUELAYER].getBoundingInformation(m_layerBBList, m_layerBBListExcludedFromCulling, &m_sceneBB);
	m_Layers[SKINNEDOPAQUELAYER].getBoundingInformation(m_layerBBList, m_layerBBListExcludedFromCulling, &m_sceneBB);
	m_Layers[SKINNEDNOTOPAQUELAYER].getBoundingInformation(m_layerBBList, m_layerBBListExcludedFromCulling, &m_sceneBB);
	m_Layers[NOTOPAQUELAYERGH].getBoundingInformation(m_layerBBList, m_layerBBListExcludedFromCulling, nullptr);
	m_Layers[NOTOPAQUELAYERCH].getBoundingInformation(m_layerBBList, m_layerBBListExcludedFromCulling, &m_sceneBB);
	
	DirectX::BoundingBox ltempBB(XMFLOAT3(m_sceneBB.Center.x, m_sceneBB.Center.y, m_sceneBB.Center.z), m_sceneBB.Extents);
	BoundingSphere::CreateFromBoundingBox(m_sceneBSShadow, ltempBB);
	m_sceneBSShadow.Radius /= 1.0f;
}

void Scene::update()
{
	if (!m_doesItNeedUpdate) return;
		
	ContainsCount = 0;
	if (m_octreeCullingMode)
	{		
		assert(0);		
	}	
	else
	{
		// Copy at first object which should be out of Frustum Culling (e.g. Sky ) (?)
		for (int i=0; i< m_Layers.size(); i++)
			m_Layers[i].update();
		
		// Looking at Octree's bounding boxes through ViewFrustum, mark required Instances are selected. Really copy will be in TechDemo::update_objectCB() function
		m_octree->selector.LOD0_distance = 15; //30
		m_octree->selector.LOD1_distance = 80; //80
		m_octree->selector.SelectorPostition = m_camera->getPosition();
		m_octree->update(m_camera->getFrustomBoundingCameraWorld(0));
	}

	m_instancesDataReadTimes = FRAMERESOURCECOUNT;
	m_doesItNeedUpdate = false;	
}

void Scene::updateLight(float time)
{
	if (!m_lightAnimation) return;

	bool lIsNight = false;
	for (int i = 0; i < m_lights.size(); i++)
	{
		if (m_skeletonManager->isExistSkeletonNodeAnimated(m_lights[i].Name))
		{
			SkinnedData& lSkeleton = m_skeletonManager->getSkeletonNodeAnimated(m_lights[i].Name);

			XMFLOAT4X4 &lFinalMatrix = lSkeleton.getNodeTransform(time, 0);
			XMMATRIX lLcTr = DirectX::XMLoadFloat4x4(&lFinalMatrix);
			if (XMMatrixIsIdentity(lLcTr)) continue;

			DirectX::XMVECTOR lPos = XMVectorSet(0.2f, 0.2f, 0.2f, 1.0f);
			DirectX::XMVECTOR lLightDirection = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

			lPos = XMVector3Transform(lPos, lLcTr);
			lLightDirection = XMVector3Normalize(XMVector3TransformNormal(lLightDirection, lLcTr));

			XMStoreFloat3(&m_lights[i].Position, lPos);
			XMStoreFloat3(&m_lights[i].Direction, lLightDirection);
			XMStoreFloat3(&m_lights[i].initDirection, lLightDirection);

			if (m_lights[i].lightType == LightType::Directional)
			{
				DirectX::XMVECTOR lWorldDown = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
				float cosA = XMVectorGetX(DirectX::XMVector3Dot(lWorldDown, lLightDirection));

				float lSaturateVal = min(max(cosA, 0.0f), 1.0f);
				float lLightIntensity = lSaturateVal;
				m_lights[i].Intensity = lLightIntensity;

				float lRed = m_lights[i].bkColor.x;
				float lGreen = m_lights[i].bkColor.y;
				float lBlue = m_lights[i].bkColor.z;

				if (m_isAfternoon)
				{
					XMFLOAT3 lDeclineColor = XMFLOAT3(1.0f, 0.4f, 0.6f);
					//XMFLOAT3 lDeclineColor = XMFLOAT3(1.0f, 0.0f, 0.0f);

					float lRedDiff = m_lights[i].bkColor.x - lDeclineColor.x;
					float lGreenDiff = m_lights[i].bkColor.y - lDeclineColor.y;
					float lBlueDiff = m_lights[i].bkColor.z - lDeclineColor.z;

					lRed -= lRedDiff * (1 - cosA) * (1 - cosA);
					lGreen -= lGreenDiff * (1 - cosA) * (1 - cosA);
					lBlue -= lBlueDiff * (1 - cosA) * (1 - cosA);
				}				

				//m_lights[i].Color = XMFLOAT3(lRed, lGreen, lBlue);

				if (cosA < 0.25f) // >75 grad
				{
					lIsNight = true;						
				}
				else
				{
					lIsNight = false;					
				}

				if (cosA < 0.01f) 
				{
					m_isAfternoon = false;
					m_prevCosA = 0;
				}

				if (!lIsNight && !m_isAfternoon)
				{
					if (cosA >= m_prevCosA)
						m_prevCosA = cosA;
					else
						m_isAfternoon = true;
				}
					
			}
		}
	}

	for (int i = 0; i < m_lights.size(); i++)
	{
		if (m_lights[i].lightType == LightType::Point)
		{
			if (lIsNight)
			{
				m_lights[i].Intensity = 0.5f + 0.2f * (float)rand() / (float)RAND_MAX;
				m_lights[i].turnOn = true;
			}
			else
				m_lights[i].turnOn = false;
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

const std::vector<LightCPU>& Scene::getLights()
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

		LightCPU tmpLight = {};
		tmpLight.lightType = LightType::Directional;
		tmpLight.mPhi = 0;
		tmpLight.mTheta = 0;
		tmpLight.mRadius = 0;

		XMStoreFloat3(&tmpLight.Direction, direction);
		XMStoreFloat3(&tmpLight.initDirection, direction);
		XMStoreFloat3(&tmpLight.Position, pos);

		tmpLight.Color = DirectX::XMFLOAT3(0.8f, 0.8f, 0.8f);

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

void Scene::SceneLayer::clearLayer()
{
	//m_objects.clear();

	for (int i = 0; i < m_objects.size(); i++)
		m_objects[i].clearInstancesLODSize();
}

void Scene::SceneLayer::init(const std::vector<std::unique_ptr<RenderItem>>& arrayRI)
{
	// for each RI for this Layer, let's create SceneLayerObject
	for (int ri = 0; ri < arrayRI.size(); ri++)
	{
		SceneLayer::SceneLayerObject lSceneObject = {};
		RenderItem* lRI = arrayRI[ri].get();		
		lSceneObject.init(lRI);		
		addSceneObject(lSceneObject);
	}
}


bool Scene::SceneLayer::isLayerVisible()
{
	return m_visible;
}

void Scene::SceneLayer::setVisibility(bool b)
{
	m_visible = b;
}

UINT Scene::SceneLayer::getLayerInstancesCount()
{
	UINT lInstancesCount = 0;
	for (int i = 0; i < m_objects.size(); i++)
		lInstancesCount += m_objects[i].getInstancesCount();

	return lInstancesCount;
}

void Scene::SceneLayer::getInstances(std::vector<const InstanceDataGPU*>& out_Instances, UINT& intancesCount)
{
	for (int i = 0; i < m_objects.size(); i++)
		m_objects[i].getInstances(out_Instances, intancesCount);
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

void Scene::SceneLayer::update()
{
	clearLayer();

	//Copy Instances which should not be in Frustum Culling
	for (int i = 0; i < m_objects.size(); i++)
		m_objects[i].copyInstancesWithoutFC();
}

//TO_DO: delete this method?
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

void Scene::SceneLayer::update(const std::vector<std::unique_ptr<RenderItem>>& arrayRI, bool LODUsing)
{
	/*
		- This variant is for using LOD for SceneLayerObject
		- Does not use DrawInstancesID, but direct ID
	*/

	if (!isLayerVisible()) return; // no need to fill this layer if it is not visible
		
	clearLayer();

	//for (int ri = 0; ri < arrayRI.size(); ri++)
	//{
	//	SceneLayer::SceneLayerObject& lSceneObject = m_objects[ri];
	//	RenderItem* lRI = arrayRI[ri].get();
	//					
	//	for (int i = 0; i < lRI->InstancesID_LOD_size; i++)
	//	{
	//		auto lID_LOD = lRI->InstancesID_LOD[i];
	//		
	//		lSceneObject.addInstance(&lRI->Instances[lID_LOD.first], lID_LOD.second);
	//	}

	//	lRI->InstancesID_LOD_size = 0;
	//}
}

void Scene::SceneLayer::getBoundingInformation(
	vector<BoundingMath::BoundingBoxEXT*>& lLayerBBList, 
	vector<BoundingMath::BoundingBoxEXT*>& lLayerBBListExcludedFromCulling,
	BoundingMath::BoundingBox* sceneBB)
{	
	//Get a list of BoundingBoxed for current SceneLayer. This BBs are in World space.

	for (int i = 0; i < m_objects.size(); i++)
		m_objects[i].getBoundingInformation(lLayerBBList, lLayerBBListExcludedFromCulling, sceneBB);
}

// ================================================================ [Scene::SceneLayer::SceneLayerObject] =============
//Scene::SceneLayer::SceneLayerObject::SceneLayerObject()
//{	
//}

void Scene::SceneLayer::SceneLayerObject::setObjectMesh(RenderItem* objectMesh)
{
	m_mesh = objectMesh;
}

void Scene::SceneLayer::SceneLayerObject::clearInstances()
{
	m_instances.clear();
}

void Scene::SceneLayer::SceneLayerObject::clearInstancesLODSize()
{
	for (int i = 0; i < RI_LOD_COUNT; i++)
		m_mesh->InstancesID_LOD_size[i] = 0;
}

void Scene::SceneLayer::SceneLayerObject::init(RenderItem* RI)
{
	// Set Mesh for this SceneLayerObject
	setObjectMesh(RI);		

	
	UINT lInstancesCount = RI->Instances.size();
	for (int i = 0; i < RI_LOD_COUNT; i++)
		RI->InstancesID_LOD[i].resize(lInstancesCount);	

	// Init data for storing Instances information
	{
		int lCountLOD = (RI->Geometry != NULL) ? 1 : 3; // If we have our mesh in RI->Geometry, so we do not have LOD for this RI, so we have only one LOD0
			
		for (int i = 0; i < lCountLOD; i++)
		{
			m_instancesLOD[i].resize(lInstancesCount);
			m_instancesLODArraySize[i] = 0;
		}
	}
}

void Scene::SceneLayer::SceneLayerObject::addInstance(const InstanceDataGPU* instance)
{
	m_instances.push_back(instance);
}

void Scene::SceneLayer::SceneLayerObject::addInstance(const InstanceDataGPU* instance, UINT LodID)
{
	m_instancesLOD[LodID][m_instancesLODArraySize[LodID]++] = instance;
}

void Scene::SceneLayer::SceneLayerObject::copyInstancesWithoutFC()
{
	if (m_mesh->ExcludeFromCulling)
		for (int i = 0; i < m_mesh->Instances.size(); i++)
			m_mesh->InstancesID_LOD[0][m_mesh->InstancesID_LOD_size[0]++] = i;
}


inline UINT Scene::SceneLayer::SceneLayerObject::getInstancesCountLOD()
{
	UINT result =0;
	for (int i = 0; i < RI_LOD_COUNT; i++)
		result += m_mesh->InstancesID_LOD_size[i];
	return result;
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

void Scene::SceneLayer::SceneLayerObject::getInstances(std::vector<const InstanceDataGPU*>& out_Instances, UINT& instancesCount)
{
	// provide instances which have been marked to be drawn.

	if (getInstancesCountLOD() == 0) return;

	for (int lod_id = 0; lod_id < LODCOUNT; lod_id++) // How many LOD level we have
		for (int i = 0; i < m_mesh->InstancesID_LOD_size[lod_id]; i++) // How many this LOD level has Objects for this SceneLayerObject
			out_Instances[instancesCount++] = &m_mesh->Instances[m_mesh->InstancesID_LOD[lod_id][i]];
}

void Scene::SceneLayer::SceneLayerObject::getBoundingInformation(
	std::vector<BoundingMath::BoundingBoxEXT*>& lLayerBBList,
	std::vector<BoundingMath::BoundingBoxEXT*>& lLayerBBListExcludedFromCulling,
	BoundingMath::BoundingBox* sceneBB)
{
/*
	+ Get All RenderItems/Instances for current SceneLayer. We divide RenderItems to list:
	- Which should in Frustom Culling (and added to Octree or Selector)
	- Which should not be Frustom Culling (for the Sky and the Land)

	+ Add BB to common scene_BB to create main BoundingBox for scene

	Result of this function are:
		- a list of BB for all Instances in Scene. This BBs are in World space.
		- a scene BB
*/	
	int lInstanceCount = m_mesh->Instances.size();

	for (int i = 0; i < lInstanceCount; i++) // All Instances for this unique mesh
	{
		BoundingBox ltBB;
		XMMATRIX lWorld = XMLoadFloat4x4(&m_mesh->Instances[i].World);
		m_mesh->AABB.Transform(ltBB, lWorld);
		BoundingMath::BoundingBoxEXT* lBBExt = new BoundingMath::BoundingBoxEXT(ltBB);
		lBBExt->pRenderItem = m_mesh;
		lBBExt->Inst_ID = i;

		if (m_mesh->ExcludeFromCulling)
			lLayerBBListExcludedFromCulling.push_back(lBBExt); // for list we do not need BB actually, but we need RI_id and Instance_ID, so just use BB for generallization
		else
			lLayerBBList.push_back(lBBExt);

		// Add this BB to sceneBB, if this required
		if (sceneBB && m_mesh->ExcludeFromCulling==false)
			BoundingMath::BoundingBox::CreateMerged(*sceneBB, *sceneBB, ltBB);
	}
}