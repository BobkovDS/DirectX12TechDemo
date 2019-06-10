#pragma once
#include "ApplDataStructures.h"
#include <map>

class ObjectManager
{
	std::vector<std::unique_ptr<RenderItem>> m_opaqueLayer;
	std::vector<std::unique_ptr<RenderItem>> m_notOpaqueLayer;
	std::vector<std::unique_ptr<RenderItem>> m_skinnedOpaqueLayer;
	std::vector<std::unique_ptr<RenderItem>> m_skinnedNotOpaqueLayer;

	std::map<std::string, std::unique_ptr<Mesh>> m_meshes;

	ObjectManager(const ObjectManager& p)=delete;
	ObjectManager& operator=(const ObjectManager* p) = delete;
public:
	ObjectManager();
	~ObjectManager();

	void addOpaqueObject(std::unique_ptr<RenderItem>& object);
	void addTransparentObject(std::unique_ptr<RenderItem>& object);
	void addSkinnedOpaqueObject(std::unique_ptr<RenderItem>& object);
	void addSkinnedNotOpaqueObject(std::unique_ptr<RenderItem>& object);
	
	void addMesh(std::string name, std::unique_ptr<Mesh>& iMesh);
	const std::vector<std::unique_ptr<RenderItem>>& getOpaqueLayer();
	const std::vector<std::unique_ptr<RenderItem>>& getNotOpaqueLayer();
	const std::vector<std::unique_ptr<RenderItem>>& getSkinnedOpaqueLayer();
	const std::vector<std::unique_ptr<RenderItem>>& getSkinnedNotOpaqueLayer();
	UINT getCommonInstancesCount();
};

