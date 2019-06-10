#pragma once
#include "ApplDataStructures.h"
#include "Defines.h"
#include "ObjectManager.h"


class Scene
{
	class SceneLayer;
	std::vector<SceneLayer> m_Layers;
	ObjectManager* m_objectManager;

	bool m_doesItNeedUpdate;
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
			std::vector<const InstanceDataGPU*>& getInstances();
			void clearInstances();
			void addInstance(const InstanceDataGPU*);
			void setObjectMesh(const RenderItem* mesh);
		};
		//-------------------------------------------------------
		void clearLayer();
		bool isLayerVisible();
		void setVisibility(bool b);
		int getLayerInstancesCount();
		int getSceneObjectCount();
		void addSceneObject(SceneLayerObject sceneObject);
		SceneLayerObject* getSceneObject(UINT sceneObjectIndex);
	};
	//----------------------------------------------------
	Scene();
	~Scene();

	int getLayersCount();
	int getLayerInstanceOffset(UINT layerIndex);
	void build(ObjectManager* objectManager);
	void update();
	SceneLayer* getLayer(UINT layerIndex);
	void updateLayer(SceneLayer& layer, const std::vector<std::unique_ptr<RenderItem>>& RI);
};

