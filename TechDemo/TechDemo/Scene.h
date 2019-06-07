#pragma once
#include "ApplDataStructures.h"
#include "Defines.h"


class Scene
{
	class SceneLayer;
	std::vector<SceneLayer> m_Layers;

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
		bool isLayerVisible();
		int getLayerInstancesCount();
		int getSceneObjectCount();
		SceneLayerObject* getSceneObject(UINT sceneObjectIndex);
	};
	//----------------------------------------------------
	Scene();
	~Scene();

	int getLayersCount();
	int getLayerInstanceOffset(UINT layerIndex);
	SceneLayer* getLayer(UINT layerIndex);
};

