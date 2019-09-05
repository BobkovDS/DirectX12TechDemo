#pragma once
#include <fbxsdk.h>
#include "ObjectManager.h"
#include "ResourceManager.h"
#include "SkeletonManager.h"
#include "Utilit3D.h"
#include "ApplLogger.h"
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
	std::vector<DirectX::XMFLOAT3> Tangents;
	std::vector<DirectX::XMFLOAT2> UVs;
	std::vector<INT32> Indices;
	bool WasUploaded;
	bool ExcludeFromCulling;
	bool ExcludeFromMirrorReflection;
	bool DoNotDublicateVertices;	
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
	std::vector<std::pair<std::string, std::string>> TexturesNameByType; //<texture_type, texture_name>
	float WaterV2_Width;
	float WaterV2_Height;
	float WaterV2_BlocksCountX;
	float WaterV2_Velocity;
	float WaterV2_TimeInterval;
	float WaterV2_Viscosity;

	bool IsTransparencyUsed; //here Transparency Factor as texture is used
	bool IsTransparent; // here Transparency Factor as float is used
	bool IsWater; // a water created on Tesselation stage
	bool IsWaterV2; // a water colculated in Compute shader
	bool IsSky;	
	bool DoesIncludeToWorldBB;
	bool ExcludeFromMirrorReflection;

};

struct fbx_NodeInstance {
	std::string MeshName;
	fbx_NodeType Nodetype;
	bool Visible;	
	std::vector<fbx_Material*> Materials;
	DirectX::XMFLOAT3 Transformation;
	DirectX::XMFLOAT3 Translation;
	DirectX::XMFLOAT4X4 GlobalTransformation;	
};

struct fbx_TreeBoneNode
{
	std::string Name;
	fbxsdk::FbxNode* Node;
	std::vector<fbx_TreeBoneNode*> Childs;
};

class FBXFileLoader
{
	void initLoader();
private:
	static int m_materialCountForPrevCalling;
	int m_materialCountForThisCall;
	FbxManager* m_sdkManager;
	FbxScene* m_scene;
	ApplLogger* m_logger;

	//static int m_lastMaterialID;
	ObjectManager* m_objectManager;
	ResourceManager* m_resourceManager;	
	SkeletonManager* m_skeletonManager;
	bool m_initialized;	

	std::string m_sceneName;
	std::map<std::string, std::unique_ptr<fbx_Mesh>> m_meshesByName;
	std::map<std::string, std::vector<fbx_Mesh*>> m_LODGroupByName;
	std::map<std::string, std::unique_ptr<RenderItem>> m_RenderItems;
	std::vector<fbx_NodeInstance> m_NodeInstances;
	std::map<std::string, fbx_Material> m_materials; //all materials for all nodes in one place
	std::map<std::string, std::string> m_texturesPath;	
	std::map<std::string, std::pair<int, fbx_TreeBoneNode*>> m_BonesIDByName;
	std::vector<FbxNode*> m_bones; //the list of bones, ordered by Bone ID
	std::vector<fbx_TreeBoneNode*> m_rootBones; // We can have some amount of Skeleton
	std::vector<fbx_TreeBoneNode*> m_cameraNodes; // "Bones" for Camera, we need it to fill Camera Animation to Skeleton object
	std::vector<fbx_TreeBoneNode*> m_lightsNodes; // "Bones" for Lights, we need it to fill Lights Animation to Skeleton object

	//std::unique_ptr<FbxSkinnedData> m_fbxSkinnedData;
	UINT m_materialLastAddedID;
	int m_BoneGlobalID;
	//std::unique_ptr<MeshGeometry> m_geoMesh; is not used in this variant meshes storing

	void createScene();
	void build_GeoMeshes();
	void build_LODGroups();
	void build_GeoMeshesLOD(fbx_Mesh* iMesh, int LODLevel, std::string& RIName);
	void build_Materials(std::string& pMaterialName);	
	void build_Animation();	
	void build_Skeleton();
	void add_SkeletonBone(SkinnedData& skeleton, fbx_TreeBoneNode* parentNode);
	void add_InstanceToRenderItem(const fbx_NodeInstance& nodeRI);
	void add_AnimationStack(FbxAnimStack* animationStack);
	bool add_AnimationInfo(FbxAnimLayer* animationLayer, SkinnedData* skeleton, fbx_TreeBoneNode* bone, std::string& animationName);
	FbxAnimCurve* get_AnimationCurve(FbxNode* node, FbxAnimLayer* animationLayer);
	void get_BindMatrix(std::string boneName, DirectX::XMFLOAT4X4& m);	
	void process_NodeInstances();
	void process_Skeleton(const FbxNode* pNode, fbx_TreeBoneNode* parent);
	void move_RenderItems();	

	int getNextMaterialID();
	void convertFbxMatrixToFloat4x4(FbxAMatrix& fbxm, DirectX::XMFLOAT4X4& m);	
	void convertFbxVector4ToFloat4(FbxVector4& fbxv, DirectX::XMFLOAT4& v);	

	std::string create_WaterV2Mesh(fbx_Material* material, const FbxNodeAttribute* pNodeAtribute);

	// FBX process functions
	void process_node(const FbxNode* pNode);
	void process_node_LOD(const FbxNode* pNode);

	void process_node_getMaterial(const FbxNode* pNode, fbx_NodeInstance& newNodeInstance);
	std::string process_mesh(const FbxNodeAttribute* pNodeAtribute, bool meshForTesselation=false,
		int LOD_level=-1, std::string* groupName=NULL);

	void process_camera(const FbxNodeAttribute* pNodeAtribute, FbxNode* pNode);
	void process_light(const FbxNodeAttribute* pNodeAtribute, FbxNode* pNode);
	bool read_texture_data(fbx_Material* destMaterial, FbxProperty* matProperty, std::string textureType); //FbxSurfacePhong* srcMaterial,

	template<class T>
	inline void add_quadInfo_to_mesh(std::vector<T>&, T t0, T t1, T t2, T t3, bool isTesselated=false);

	template<class T>
	inline void build_GeoMeshesWithTypedVertex(fbx_Mesh* iMesh, bool skinnedMesh);
	
	inline void addSkinnedInfoToVertex(VertexExtGPU& vertex, fbx_Mesh* iMesh, int vi);
	inline void addSkinnedInfoToVertex(VertexGPU& vertex, fbx_Mesh* iMesh, int vi);
public:
	
	FBXFileLoader();
	~FBXFileLoader();

	void Initialize(ObjectManager* imngrObject, ResourceManager* mngrResource, SkeletonManager* mngrSkeleton);
	void loadSceneFile(const std::string& fileName);
};

