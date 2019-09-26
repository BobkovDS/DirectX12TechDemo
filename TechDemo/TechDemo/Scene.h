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
	std::vector<UINT> m_tmp_drawInstancesID;
	ObjectManager* m_objectManager;
	SkeletonManager* m_skeletonManager;
	Camera* m_camera;
	Octree* m_octree;	

	std::vector<LightCPU> m_lights; //lights in the scene
	DirectX::BoundingSphere m_sceneBS; // scene bounding sphere  TO_DO: Delete
	DirectX::BoundingSphere m_sceneBSShadow; // scene bounding sphere for creation Sun-light Ortho Frustom 
	BoundingMath::BoundingBox m_sceneBB;
	std::vector<BoundingMath::BoundingBoxEXT*> m_layerBBList;
	std::vector<BoundingMath::BoundingBoxEXT*> m_layerBBListExcludedFromCulling;

	bool m_doesItNeedUpdate;
	bool m_firstBB;
	bool m_lightAnimation;
	bool m_octreeCullingMode;
	int m_instancesDataReadTimes; // How many times we need provide Instances Data for FrameResource Manager;
	int m_instancesToDraw; // How many Instances are "visible" in this Scene		
	
	void buildOctree();
	
	//some for visualization drawing logic 
	bool m_isAfternoon;
	float m_prevCosA;
public:
	static UINT ContainsCount;
	class SceneLayer
	{
	public:
		class SceneLayerObject
		{			
			RenderItem* m_mesh;
			std::vector<const InstanceDataGPU*> m_instances; // all Instances: for Shadowing and for Drawing
			std::vector<const InstanceDataGPU*> m_instancesLOD[LODCOUNT];
			UINT m_instancesLODArraySize[LODCOUNT];

			std::vector<UINT> m_drawInstancesID; // Instances ID in m_instances for drawing // TO_DO: delete
			UINT m_drawInstanceIDCount;
		public:
			//SceneLayerObject();
			inline const RenderItem* getObjectMesh();
			void getInstances(std::vector<const InstanceDataGPU*>& out_Instances, 
				std::vector<UINT>& out_DrawInstancesID, UINT InstancesPerPrevLayer);			
			void getInstances(std::vector<const InstanceDataGPU*>& out_Instances, UINT& instancesCount);
			UINT getInstancesCount() { return (UINT) m_mesh->Instances.size(); }
			inline UINT getInstancesCountLOD();
			inline UINT getInstancesCountLOD_byLevel(UINT levelID);
			UINT getDrawInstancesIDCount() { return (UINT) m_drawInstancesID.size(); }
			void clearInstances();
			void clearInstancesLODSize();
			void init(RenderItem* RI);
			void addInstance(const InstanceDataGPU*);
			void addInstance(const InstanceDataGPU*, UINT LodID);
			void copyInstancesWithoutFC(); 
			void setObjectMesh(RenderItem* mesh);
			void setMaxSizeForDrawingIntancesArray(UINT maxSize) { m_drawInstancesID.resize(maxSize); }
			void setMinSizeForDrawingIntancesArray() { m_drawInstancesID.resize(m_drawInstanceIDCount); }
			void setDrawInstanceID(UINT id) { m_drawInstancesID[m_drawInstanceIDCount++] = id; };			
			const std::vector<UINT>& getDrawIntancesID() { return m_drawInstancesID; }
			void getBoundingInformation(std::vector<BoundingMath::BoundingBoxEXT*>& lLayerBBList, std::vector<BoundingMath::BoundingBoxEXT*>& lLayerBBListExcludedFromCulling,
				BoundingMath::BoundingBox* sceneBB);
		};
		//-------------------------------------------------------
		void clearLayer();	
		void init(const std::vector<std::unique_ptr<RenderItem>>& RI);
		void update(const std::vector<std::unique_ptr<RenderItem>>& RI);
		void update(const std::vector<std::unique_ptr<RenderItem>>& RI, bool LODUsing);
		void update();
		bool isLayerVisible();
		void setVisibility(bool b);
		UINT getLayerInstancesCount();
		void getInstances(std::vector<const InstanceDataGPU*>& out_Instances, std::vector<UINT>& out_DrawInstancesID, 
			UINT InstancesPerPrevLayer);
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
	int getLayerInstanceOffset(UINT layerIndex);
	std::vector<const InstanceDataGPU*>& getInstancesUpdate(UINT& instancesCount);	
	std::vector<const InstanceDataGPU*>& getInstances();	
	std::vector<UINT>& getDrawInstancesID();
	

	const std::vector<LightCPU>& getLights();
	const BoundingMath::BoundingBox& getSceneBB() { return m_sceneBB; }	
	const DirectX::BoundingSphere& getSceneBS() { return m_sceneBS; } // TO_DO: Delete
	const DirectX::BoundingSphere& getSceneBSShadow() { return m_sceneBSShadow; }
	bool getCullingModeOctree() { return m_octreeCullingMode; }
	void build(ObjectManager* objectManager, Camera* camera, SkeletonManager* skeletonManagers);
	void InitLayers();
	void update();
	void updateLight(float time);
	bool isInstancesDataUpdateRequred();
	void cameraListener();
	void toggleLightAnimation();
	void toggleCullingMode();
	int getSelectedInstancesCount() { return m_instancesToDraw; }
	UINT getTrianglesCount();
};

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