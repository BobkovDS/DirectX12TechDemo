#pragma once
#include "ApplDataStructures.h"
#include "Defines.h"
#include "ObjectManager.h"
#include "SkeletonManager.h"
#include "Camera.h"
#include "Octree.h"
#include "Selector.h"

#define FRAMERESOURCECOUNT 3
#define LODCOUNT 3
class Scene
{
	class SceneLayer;
	std::vector<SceneLayer> m_Layers;
	std::vector<const InstanceDataGPU*> m_tmp_Intances;
	std::vector<UINT> m_tmp_drawInstancesID;
	ObjectManager* m_objectManager;
	SkeletonManager* m_skeletonManager;
	Camera* m_camera;
	Octree* m_octree;
	Selector m_selector;

	std::vector<CPULight> m_lights; //lights in the scene
	DirectX::BoundingSphere m_sceneBS; // scene bounding sphere 
	DirectX::BoundingSphere m_sceneBSShadow; // scene bounding sphere for creation Sun-light Ortho Frustom 
	DirectX::BoundingBox m_sceneBB;
	DirectX::BoundingBox m_sceneBBShadow;
	std::vector<BoundingBoxEXT*> m_layerBBList;

	bool m_doesItNeedUpdate;
	bool m_firstBB;
	bool m_lightAnimation;
	bool m_octreeCullingMode;
	int m_instancesDataReadTimes; // How many times we need provide Instances Data for FrameResource Manager;
	
	void updateLayer(SceneLayer& layer, const std::vector<std::unique_ptr<RenderItem>>& RI, bool isFrustumCullingRequired = true);
	void createBoungingInfo();
	void getLayerBoundingInfo(DirectX::BoundingBox& layerBB, DirectX::BoundingBox& layerBBShadow,
		const std::vector<std::unique_ptr<RenderItem>>& RI);
	
	void buildOctree();
	void addOctreeInformation(const std::vector<std::unique_ptr<RenderItem>>& arrayRI, std::vector<BoundingBoxEXT*>& lLayerBBList);

public:
	static UINT ContainsCount;
	class SceneLayer
	{
		class SceneLayerObject;
		std::vector<SceneLayerObject> m_objects;
		bool m_visible;
	public:
		class SceneLayerObject
		{			
			const RenderItem* m_mesh;
			std::vector<const InstanceDataGPU*> m_instances; // all Instances: for Shadowing and for Drawing
			std::vector<const InstanceDataGPU*> m_instancesLOD[LODCOUNT];
			UINT m_instancesLODArraySize[LODCOUNT];

			std::vector<UINT> m_drawInstancesID; // Instances ID in m_instances for drawing
			UINT m_drawInstanceIDCount;
		public:
			//SceneLayerObject();
			inline const RenderItem* getObjectMesh();
			void getInstances(std::vector<const InstanceDataGPU*>& out_Instances, 
				std::vector<UINT>& out_DrawInstancesID, UINT InstancesPerPrevLayer);			
			void getInstances(std::vector<const InstanceDataGPU*>& out_Instances);
			UINT getInstancesCount() { return m_instances.size(); }
			inline UINT getInstancesCountLOD();
			inline UINT getInstancesCountLOD_byLevel(UINT levelID);
			UINT getDrawInstancesIDCount() { return m_drawInstancesID.size(); }
			void clearInstances();
			void clearInstancesLODSize();
			void init(RenderItem* RI);
			void addInstance(const InstanceDataGPU*);
			void addInstance(const InstanceDataGPU*, UINT LodID);
			void setObjectMesh(const RenderItem* mesh);
			void setMaxSizeForDrawingIntancesArray(UINT maxSize) { m_drawInstancesID.resize(maxSize); }
			void setMinSizeForDrawingIntancesArray() { m_drawInstancesID.resize(m_drawInstanceIDCount); }
			void setDrawInstanceID(UINT id) { m_drawInstancesID[m_drawInstanceIDCount++] = id; };			
			const std::vector<UINT>& getDrawIntancesID() { return m_drawInstancesID; }
		};
		//-------------------------------------------------------
		void clearLayer();	
		void init(const std::vector<std::unique_ptr<RenderItem>>& RI);
		void update(const std::vector<std::unique_ptr<RenderItem>>& RI);
		void update(const std::vector<std::unique_ptr<RenderItem>>& RI, bool LODUsing);
		bool isLayerVisible();
		void setVisibility(bool b);
		int getLayerInstancesCount();
		void getInstances(std::vector<const InstanceDataGPU*>& out_Instances, std::vector<UINT>& out_DrawInstancesID, 
			UINT InstancesPerPrevLayer);
		void getInstances(std::vector<const InstanceDataGPU*>& out_Instances);
		int getSceneObjectCount();
		void addSceneObject(SceneLayerObject sceneObject);
		SceneLayerObject* getSceneObject(UINT sceneObjectIndex);
	};
	//----------------------------------------------------
	Scene();
	~Scene();

	int getLayersCount();
	SceneLayer* getLayer(UINT layerIndex);
	int getLayerInstanceOffset(UINT layerIndex);
	std::vector<const InstanceDataGPU*>& getInstancesUpdate();	
	std::vector<const InstanceDataGPU*>& getInstances();	
	std::vector<UINT>& getDrawInstancesID();
	

	const std::vector<CPULight>& getLights();
	const DirectX::BoundingBox& getSceneBB() { return m_sceneBB; }
	const DirectX::BoundingBox& getSceneBBShadow() { return m_sceneBBShadow; }
	const DirectX::BoundingSphere& getSceneBS() { return m_sceneBS; }
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
};

inline const RenderItem* Scene::SceneLayer::SceneLayerObject::getObjectMesh()
{
	return m_mesh;
}

inline UINT Scene::SceneLayer::SceneLayerObject::getInstancesCountLOD_byLevel(UINT levelID)
{
	if (levelID >= LODCOUNT) return 0;
	else
		return m_instancesLODArraySize[levelID];
}