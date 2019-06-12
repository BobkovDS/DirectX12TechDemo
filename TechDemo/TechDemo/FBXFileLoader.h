#pragma once
#include <fbxsdk.h>
#include "ObjectManager.h"
#include "ResourceManager.h"
#include "SkeletonManager.h"
#include "Utilit3D.h"
#include <map>

enum fbx_NodeType {
	NT_Mesh = 'mesh',
	NT_Light,
	NT_Camera
};

struct fbx_Mesh {
	std::string Name;
	std::vector<DirectX::XMFLOAT3> Vertices;
	std::vector<std::vector<std::pair<std::string, float>>> VertexWeightByBoneName;
	std::vector<DirectX::XMFLOAT3> Normals;
	std::vector<DirectX::XMFLOAT2> UVs;
	std::vector<INT32> Indices;
	std::vector<INT32> Materials;
	int VertexPerPolygon;
};

struct fbx_Material {
	std::string Name;
	int MatCBIndex = -1;

	//Material Constant Buffer data
	DirectX::XMFLOAT4 DiffuseAlbedo;
	DirectX::XMFLOAT4 Specular;
	DirectX::XMFLOAT4 TransparencyColor;
	
	DirectX::XMFLOAT3 FresnelR0 = { 0.5f, 0.1f, 0.1f };
	float Roughness = 0.1f;
	DirectX::XMFLOAT4X4 MatTransform;
	std::vector<std::pair<std::string, std::string>> TexturesNameByType;
	bool IsTransparencyUsed; //here Transparency Factor as texture is used
	bool IsTransparent; // here Transparency Factor as float is used
};

struct fbx_NodeInstance {
	std::string MeshName;
	fbx_NodeType Nodetype;
	std::vector<fbx_Material*> Materials;
	DirectX::XMFLOAT3 Transformation;
	DirectX::XMFLOAT3 Translation;
	DirectX::XMFLOAT4X4 GlobalTransformation;
	DirectX::XMFLOAT4X4 LocalTransformation;
};

struct fbx_TreeBoneNode
{
	std::string Name;
	fbxsdk::FbxNode* Node;
	std::vector<fbx_TreeBoneNode*> Childs;
};

class FBXFileLoader
{
private:
	FbxManager* m_sdkManager;
	FbxScene* m_scene;

	//static int m_lastMaterialID;
	ObjectManager* m_objectManager;
	ResourceManager* m_resourceManager;	
	SkeletonManager* m_skeletonManager;
	bool m_initialized;

	std::string m_sceneName;
	std::map<std::string, std::unique_ptr<fbx_Mesh>> m_meshesByName;
	std::map<std::string, std::unique_ptr<RenderItem>> m_RenderItems;
	std::vector<fbx_NodeInstance> m_NodeInstances;
	std::map<std::string, fbx_Material> m_materials; //all materials for all nodes in one place
	std::map<std::string, std::string> m_texturesPath;	
	std::map<std::string, std::pair<int, fbx_TreeBoneNode*>> m_BonesIDByName;
	std::vector<FbxNode*> m_bones; //the list of bones, ordered by Bone ID
	std::vector<fbx_TreeBoneNode*> m_rootBones; // We can have some amount of Skeleton
	//std::unique_ptr<FbxSkinnedData> m_fbxSkinnedData;
	UINT m_materialLastAddedID;
	int m_BoneGlobalID;
	//std::unique_ptr<MeshGeometry> m_geoMesh; is not used in this variant meshes storing

	void createScene();
	void build_GeoMeshes();
	void build_Materials(std::string& pMaterialName);	
	void build_Animation();	
	void build_Skeleton();
	void add_SkeletonBone(SkinnedData& skeleton, fbx_TreeBoneNode* parentNode);
	void add_InstanceToRenderItem(const fbx_NodeInstance& nodeRI);
	void add_AnimationStack(FbxAnimStack* animationStack);
	void add_AnimationInfo(FbxAnimLayer* animationLayer, SkinnedData* skeleton, fbx_TreeBoneNode* bone, std::string& animationName);
	void get_BindMatrix(std::string boneName, DirectX::XMFLOAT4X4& m);
	void process_NodeInstances();
	void process_Skeleton(const FbxNode* pNode, fbx_TreeBoneNode* parent);
	void move_RenderItems();	

	int getNextMaterialID();
	void convertFbxMatrixToFloat4x4(FbxAMatrix& fbxm, DirectX::XMFLOAT4X4& m);	
	void convertFbxVector4ToFloat4(FbxVector4& fbxv, DirectX::XMFLOAT4& v);	

	// FBX process functions
	void process_node(const FbxNode* pNode);
	void process_mesh(const FbxNodeAttribute* pNodeAtribute);
	bool read_texture_data(fbx_Material* destMaterial, FbxProperty* matProperty, std::string textureType); //FbxSurfacePhong* srcMaterial,
public:
	
	FBXFileLoader();
	~FBXFileLoader();

	void Initialize(ObjectManager* imngrObject, ResourceManager* mngrResource, SkeletonManager* mngrSkeleton);
	void loadSceneFile(const std::string& fileName);
};

