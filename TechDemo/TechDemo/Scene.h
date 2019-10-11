/*
	***************************************************************************************************
	Description:
		Presents a Scene with objects. Gives some kind of abstraction under data from ObjectManager.
		It also has Layers(SceneLayer), Objects (SceneLayerObject). Connect together Octree, Frustum Culling, Objects.
		This class is data source for Renders;

		- With given Objects from ObjectManager, we fill Scene layers how we need, some Scene Layer, can be Turned Off
		- Prepare a list of Objects/Instances for Octree and build Octree
		
		- Update Scene: Using Octree, create a Instances list, which should be copied to GPU
		- Update Light
	***************************************************************************************************
*/

#pragma once
#include "ApplDataStructures.h"
#include "Defines.h"
#include "ObjectManager.h"
#include "SkeletonManager.h"
#include "Camera.h"
#include "Octree.h"

#define FRAMERESOURCECOUNT 3
#define LODCOUNT 3
class Scene
{
public: class SceneLayer;
private:
	std::vector<SceneLayer> m_Layers;
	std::vector<const InstanceDataGPU*> m_tmp_Intances;	
	ObjectManager* m_objectManager;
	SkeletonManager* m_skeletonManager;
	Camera* m_camera;
	Octree* m_octree;	

	std::vector<LightCPU> m_lights; //lights in the scene	
	BoundingMath::BoundingBox m_sceneBB;
	std::vector<BoundingMath::BoundingBoxEXT*> m_layerBBList;
	std::vector<BoundingMath::BoundingBoxEXT*> m_layerBBListExcludedFromCulling;

	bool m_doesItNeedUpdate;	
	bool m_lightAnimation;	
	int m_instancesDataReadTimes; // How many times we need provide Instances Data for FrameResource Manager;
	UINT m_instancesToDraw; // How many Instances are "visible" in this Scene		
	
	void buildOctree();
	
	//some for visualization drawing logic 
	bool m_isAfternoon;
	float m_prevCosA;
public:	
	class SceneLayer
	{
	public:
		class SceneLayerObject
		{			
			RenderItem* m_mesh;						
		public:
			//SceneLayerObject();
			inline const RenderItem* getObjectMesh();			
			void getInstances(std::vector<const InstanceDataGPU*>& out_Instances, UINT& instancesCount);
			UINT getInstancesCount() { return (UINT) m_mesh->Instances.size(); }
			inline UINT getInstancesCountLOD();
			inline UINT getInstancesCountLOD_byLevel(UINT levelID);			
			void clearInstancesLODSize();
			void init(RenderItem* RI);						
			void copyInstancesWithoutFC(); 
			void setObjectMesh(RenderItem* mesh);			
			void getBoundingInformation(std::vector<BoundingMath::BoundingBoxEXT*>& lLayerBBList, std::vector<BoundingMath::BoundingBoxEXT*>& lLayerBBListExcludedFromCulling,
				BoundingMath::BoundingBox* sceneBB);
		};
		//-------------------------------------------------------
		void clearLayer();	
		void init(const std::vector<std::unique_ptr<RenderItem>>& RI);				
		void update();
		bool isLayerVisible();
		void setVisibility(bool b);
		UINT getLayerInstancesCount();		
		void getInstances(std::vector<const InstanceDataGPU*>& out_Instances, UINT& intancesCount);
		int getSceneObjectCount();		
		void addSceneObject(SceneLayerObject sceneObject);
		SceneLayerObject* getSceneObject(UINT sceneObjectIndex);
		void getBoundingInformation(std::vector<BoundingMath::BoundingBoxEXT*>& lLayerBBList, std::vector<BoundingMath::BoundingBoxEXT*>& lLayerBBListExcludedFromCulling,
			BoundingMath::BoundingBox* sceneBB);
	private:
		std::vector<SceneLayerObject> m_objects;
		bool m_visible;
	};
	//----------------------------------------------------
	Scene();
	~Scene();

	int getLayersCount();
	SceneLayer* getLayer(UINT layerIndex);	
	void UpdateInstances();	
	std::vector<const InstanceDataGPU*>& getInstances();		

	const std::vector<LightCPU>& getLights();
	const BoundingMath::BoundingBox& getSceneBB() { return m_sceneBB; }		
	void build(ObjectManager* objectManager, Camera* camera, SkeletonManager* skeletonManagers);
	void InitLayers();
	void update();
	void updateLight(float time);
	bool isInstancesDataUpdateRequred();
	void cameraListener();
	void toggleLightAnimation();	
	UINT getSelectedInstancesCount() { return m_instancesToDraw; }	
	void decrementInstancesReadCounter() { m_instancesDataReadTimes--; }
};


// ********* Inline methods *****************

inline const RenderItem* Scene::SceneLayer::SceneLayerObject::getObjectMesh()
{
	return m_mesh;
}

inline UINT Scene::SceneLayer::SceneLayerObject::getInstancesCountLOD_byLevel(UINT levelID)
{
	if (levelID >= LODCOUNT) return 0;
	else
		return m_mesh->InstancesID_LOD_size[levelID];
}