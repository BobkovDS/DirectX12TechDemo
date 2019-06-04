#pragma once
#include "ApplDataStructures.h"
#include <map>

class ObjectManager
{
	std::vector<std::unique_ptr<RenderItem>> m_opaqueLayer;
	std::map<std::string, std::unique_ptr<Mesh>> m_meshes;

	ObjectManager(const ObjectManager& p)=delete;
	ObjectManager& operator=(const ObjectManager* p) = delete;
public:
	ObjectManager();
	~ObjectManager();

	void addOpaqueObject(std::unique_ptr<RenderItem>& object);
	void addTransparentObject(std::unique_ptr<RenderItem>& object);
	void addAnimatedObject(std::unique_ptr<RenderItem>& object);
	
	void addMesh(std::string name, std::unique_ptr<Mesh>& iMesh);
	const std::vector<std::unique_ptr<RenderItem>>& getOpaqueLayer();
};

