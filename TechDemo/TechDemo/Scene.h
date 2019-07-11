#pragma once
#include "ApplDataStructures.h"
#include "Defines.h"
#include "ObjectManager.h"
#include "SkeletonManager.h"
#include "Camera.h"

#define FRAMERESOURCECOUNT 3
class Scene
{
	class SceneLayer;
	std::vector<SceneLayer> m_Layers;
	std::vector<const InstanceDataGPU*> m_tmp_Intances;
	ObjectManager* m_objectManager;
	SkeletonManager* m_skeletonManager;
	Camera* m_camera;

	std::vector<CPULight> m_lights; //lights in the scene
	DirectX::BoundingSphere m_sceneBS; // scene bounding sphere for creation Sun-light Ortho Frustom 
	DirectX::BoundingBox m_sceneBB;

	bool m_doesItNeedUpdate;
	bool m_firstBB;
	bool m_lightAnimation;
	int m_instancesDataReadTimes; // How many times we need provide Instances Data for FrameResource Manager;
	
	void updateLayer(SceneLayer& layer, const std::vector<std::unique_ptr<RenderItem>>& RI, bool isFrustumCullingRequired = true);
	void createBoungingInfo();
	void getLayerBoundingInfo(DirectX::BoundingBox& layerBB, const std::vector<std::unique_ptr<RenderItem>>& RI);
public:
	class SceneLayer
	{
		class SceneLayerObject;
		std::vector<SceneLayerObject> m_objects;
		bool m_visible;
	public:
		class SceneLayerObject
		{			
			const RenderItem* m_mesh;
			std::vector<const InstanceDataGPU*> m_instances;
		public:
			const RenderItem* getObjectMesh();
			void getInstances(std::vector<const InstanceDataGPU*>& out_Instances);
			UINT getInstancesCount() { return m_instances.size(); }
			void clearInstances();
			void addInstance(const InstanceDataGPU*);
			void setObjectMesh(const RenderItem* mesh);
		};
		//-------------------------------------------------------
		void clearLayer();
		bool isLayerVisible();
		void setVisibility(bool b);
		int getLayerInstancesCount();
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
	const std::vector<CPULight>& getLights();
	const DirectX::BoundingBox& getSceneBB() { return m_sceneBB; }
	const DirectX::BoundingSphere& getSceneBS() { return m_sceneBS; }
	void build(ObjectManager* objectManager, Camera* camera, SkeletonManager* skeletonManagers);
	void update();
	void updateLight(float time);
	bool isInstancesDataUpdateRequred();
	void cameraListener();
	void toggleLightAnimation();
};

