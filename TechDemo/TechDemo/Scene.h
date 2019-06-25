#pragma once
#include "ApplDataStructures.h"
#include "Defines.h"
#include "ObjectManager.h"
#include "Camera.h"

#define FRAMERESOURCECOUNT 3
class Scene
{
	class SceneLayer;
	std::vector<SceneLayer> m_Layers;
	std::vector<const InstanceDataGPU*> m_tmp_Intances;
	ObjectManager* m_objectManager;
	Camera* m_camera;

	std::vector<CPULight> m_lights; //lights in the scene

	bool m_doesItNeedUpdate;
	int m_instancesDataReadTimes; // How many times we need provide Instances Data for FrameResource Manager;
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
	void build(ObjectManager* objectManager, Camera* camera);
	void update();
	void updateLayer(SceneLayer& layer, const std::vector<std::unique_ptr<RenderItem>>& RI, bool isFrustumCullingRequired=true);
	bool isInstancesDataUpdateRequred();
	void cameraListener();

};

