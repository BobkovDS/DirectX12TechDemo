#pragma once
#include "ApplDataStructures.h"
#include "Camera.h"
#include <map>

class ObjectManager
{
	std::vector<std::unique_ptr<RenderItem>> m_skyLayer; // actualy we have only one Sky
	std::vector<std::unique_ptr<RenderItem>> m_opaqueLayer;
	std::vector<std::unique_ptr<RenderItem>> m_opaqueLayerGH; // geometry shader
	std::vector<std::unique_ptr<RenderItem>> m_notOpaqueLayer;
	std::vector<std::unique_ptr<RenderItem>> m_notOpaqueLayerGH;
	std::vector<std::unique_ptr<RenderItem>> m_skinnedOpaqueLayer;
	std::vector<std::unique_ptr<RenderItem>> m_skinnedNotOpaqueLayer;

	std::map<std::string, std::unique_ptr<Mesh>> m_meshes;
	std::vector<std::unique_ptr<Camera>> m_cameras;

	ObjectManager(const ObjectManager& p)=delete;
	ObjectManager& operator=(const ObjectManager* p) = delete;
	void mirrorZLayer(std::vector<std::unique_ptr<RenderItem>>& layer, DirectX::XMMATRIX& mirrZM);
public:
	ObjectManager();
	~ObjectManager();

	void addOpaqueObject(std::unique_ptr<RenderItem>& object);
	void addTransparentObject(std::unique_ptr<RenderItem>& object);
	void addTransparentObjectGH(std::unique_ptr<RenderItem>& object);
	void addSkinnedOpaqueObject(std::unique_ptr<RenderItem>& object);
	void addSkinnedNotOpaqueObject(std::unique_ptr<RenderItem>& object);
	void addSky(std::unique_ptr<RenderItem>& object);
	void addMesh(std::string name, std::unique_ptr<Mesh>& iMesh);
	void addCamera(std::unique_ptr<Camera>& camera);
	
	const std::vector<std::unique_ptr<Camera>>& getCameras();
	const std::vector<std::unique_ptr<RenderItem>>& getSky();
	const std::vector<std::unique_ptr<RenderItem>>& getOpaqueLayer();
	const std::vector<std::unique_ptr<RenderItem>>& getNotOpaqueLayer();
	const std::vector<std::unique_ptr<RenderItem>>& getNotOpaqueLayerGH();
	const std::vector<std::unique_ptr<RenderItem>>& getSkinnedOpaqueLayer();
	const std::vector<std::unique_ptr<RenderItem>>& getSkinnedNotOpaqueLayer();
	UINT getCommonInstancesCount();
	void mirrorZ();
};

