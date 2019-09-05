#include "FBXFileLoader.h"
#include "ApplException.h"

using namespace std;
using namespace DirectX;

#define NORMALMODE // if defined, Normal is just read from file. In another case, Normal should be Averaged.

int FBXFileLoader::m_materialCountForPrevCalling = 0;

FBXFileLoader::FBXFileLoader()
{
	m_sdkManager = FbxManager::Create();
	m_materialLastAddedID = 0;
	m_materialCountForThisCall = 0;
	m_initialized = false;
	m_logger = &ApplLogger::getLogger();	
}

FBXFileLoader::~FBXFileLoader()
{
	if (m_scene != NULL) m_scene->Destroy();
	m_sdkManager->Destroy();

	for (int i = 0; i < m_rootBones.size(); i++)
		delete m_rootBones[i];

	for (int i = 0; i < m_cameraNodes.size(); i++)
		delete m_cameraNodes[i];

	for (int i = 0; i < m_lightsNodes.size(); i++)
		delete m_lightsNodes[i];
}

void FBXFileLoader::Initialize(ObjectManager* mngrObject, ResourceManager* mngrResource, SkeletonManager* mngrSkeleton)
{
	assert(mngrObject);
	assert(mngrResource);	
	assert(mngrSkeleton);

	m_objectManager = mngrObject;	
	m_resourceManager = mngrResource;	
	m_skeletonManager = mngrSkeleton;
	m_initialized = true;	
}

void FBXFileLoader::initLoader()
{
	m_meshesByName.clear();
}

void FBXFileLoader::loadSceneFile(const string& fileName)
{	
	AutoIncrementer lOutShifter;
	
	ApplLogger::getLogger().log("Loading of " + fileName, lOutShifter.Shift-1);

	if (!m_initialized) assert(0);

	// Create the IO settings object
	FbxIOSettings* ios = FbxIOSettings::Create(m_sdkManager, IOSROOT);
	m_sdkManager->SetIOSettings(ios);

	//Create an importer using the SDK manger
	FbxImporter* l_importer = FbxImporter::Create(m_sdkManager, "");

	bool res = l_importer->Initialize(fileName.c_str(), -1, m_sdkManager->GetIOSettings());
	if (!res)
	{
		std::string errMsg = l_importer->GetStatus().GetErrorString();
		throw MyCommonRuntimeException(errMsg, L"FBXFileLoader::loadSceneFlle");
	}

	if (l_importer->IsFBX())
	{
		m_sdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_MATERIAL, true);
		m_sdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_TEXTURE, true);
		m_sdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_LINK, true);
		m_sdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_SHAPE, true);
		m_sdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_GOBO, true);
		m_sdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_ANIMATION, true);
		m_sdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	int pos1 = fileName.find_last_of('\\');
	if (pos1 < 0) pos1 = 0;
	else
		pos1 += 1;
	
	int pos2 = fileName.find_last_of('.');
	string lSceneName = fileName.substr(pos1, pos2 - pos1);

	m_scene = FbxScene::Create(m_sdkManager, lSceneName.c_str());

	l_importer->Import(m_scene);
	l_importer->Destroy();

	m_scene->SetName(lSceneName.c_str());
	m_resourceManager->setPrefixName(lSceneName);
	ApplLogger::getLogger().log("Creating scene [" + lSceneName + "]", lOutShifter.Shift);
	createScene();	
	ApplLogger::getLogger().log("Loading of " + fileName + " is done", lOutShifter.Shift-1);
}

void FBXFileLoader::createScene()
{
	if (m_scene == nullptr) return;

	m_sceneName = m_scene->GetName();

	FbxGlobalSettings& lGlobalSt = m_scene->GetGlobalSettings();
	FbxSystemUnit lsu = lGlobalSt.GetSystemUnit();
	FbxAxisSystem axisSystem = lGlobalSt.GetAxisSystem();
	
	const FbxSystemUnit::ConversionOptions lConversionOptions = {
	false, /* mConvertRrsNodes */
	true, /* mConvertLimits */
	true, /* mConvertClusters */
	true, /* mConvertLightIntensity */
	true, /* mConvertPhotometricLProperties */
	true  /* mConvertCameraClipPlanes */
	};

	FbxSystemUnit::m.ConvertScene(m_scene, lConversionOptions);

	initLoader();

	int child_count = m_scene->GetRootNode()->GetChildCount();
	for (int i = 0; i < child_count; i++)
		process_node(m_scene->GetRootNode()->GetChild(i));
	
	build_LODGroups();
	build_GeoMeshes();

	m_resourceManager->buildTexturePathList();	
	process_NodeInstances();

	build_Skeleton();
	build_Animation();

	m_materialCountForPrevCalling += m_materialCountForThisCall;
}

void FBXFileLoader::build_GeoMeshes()
{
	/*
		The variant "One_RenderItem_For_One_Unique_Mesh"

		We build one MeshGeometry (Mesh), one RenderItem with this geoMesh for each unique Mesh.
		Mesh.Instances are used for instancing this mesh.

		It contains:
			1) Build all unique geoMeshes and upload it on GPU (build_GeoMeshes())
			2) Build all materials with through numbering for all unique meshes and upload it on GPU. Here (process_NodeInstances/build_Materials())
			3) Update all renderItems with required instances data and move it up (process_NodeInstances()/build_RenderItems)
	*/

	//TO_DO: Check all .end()
	auto mesh_it_begin = m_meshesByName.begin();
	for (; mesh_it_begin != m_meshesByName.end(); mesh_it_begin++)
	{
		bool lIsSkinnedMesh = false;		

		fbx_Mesh* lMesh = mesh_it_begin->second.get();
		lIsSkinnedMesh = lMesh->VertexWeightByBoneName.size() > 0;

		if (lMesh->WasUploaded == false) // it can be uploaded by LODGroups uploader
		{
			if (lIsSkinnedMesh)
				build_GeoMeshesWithTypedVertex<VertexExtGPU>(lMesh, true);
			else
				build_GeoMeshesWithTypedVertex<VertexGPU>(lMesh, false);
		}
	}
}

void FBXFileLoader::build_LODGroups()
{
	/*
		The variant: "One_RenderItem_For_One_Unique_Mesh" in LOD way:
			One RenderItem has three LOD meshes in LODGeometry[3] array

		-We build three LOD MeshGeometries (Mesh), one RenderItem with these Meshes.		
		-Mesh.Instances are used for instancing the first LOD mesh.

		It contains:
			1) Build all unique geoMeshes and upload it on GPU (build_GeoMeshes())
			2) Build all materials with through numbering for all unique meshes and upload it on GPU. Here (process_NodeInstances/build_Materials()) : TO_DO: check it
			3) Update all renderItems with required instances data and move it up (process_NodeInstances()/build_RenderItems)
	*/
		
	auto grp_it_begin = m_LODGroupByName.begin();
	auto grp_it_end= m_LODGroupByName.end();

	for (; grp_it_begin != grp_it_end; grp_it_begin++)
	{
		string lRIName = grp_it_begin->second[0]->Name; // we use the first LOD0 mesh for naming of RenderItem for it

		//Create RenderItem for these meshes	
		m_RenderItems[lRIName] = std::make_unique<RenderItem>();
		m_RenderItems[lRIName]->Name = m_sceneName + "_" + lRIName;
		build_GeoMeshesLOD(grp_it_begin->second[0], 0, lRIName);
		build_GeoMeshesLOD(grp_it_begin->second[1], 1, lRIName);
		build_GeoMeshesLOD(grp_it_begin->second[2], 2, lRIName);
	}
}

void FBXFileLoader::build_GeoMeshesLOD(fbx_Mesh* iMesh, int LODLevel, std::string& RIName)
{
	std::vector<VertexGPU> meshVertices;
	std::vector<uint32_t> meshIndices;
	VertexGPU vertex = {};

	auto lgeoMesh = std::make_unique<Mesh>();

	fbx_Mesh* lMesh = iMesh;
	SubMesh submesh = {};

	std::string lInnerRIName = lMesh->Name;
	std::string lOuterRIName = m_sceneName + "_" + lInnerRIName;

	lgeoMesh->Name = lInnerRIName;

	if (lMesh->DoNotDublicateVertices)
		assert(0); // mesh for LOD should not ask about "Not creation" of dublicate vertices

	meshVertices.resize(lMesh->Indices.size()); /* Vertices count = Indices count,
												because a Vertex can be the same for several faces, but this Vertex should
												have different(unique) information for this faces like Normal, TangentUV*/
	meshIndices.resize(lMesh->Indices.size());

	for (int vi = 0; vi < lMesh->Indices.size(); vi++)
	{
		vertex = {};
		vertex.Pos = lMesh->Vertices[lMesh->Indices[vi]];

		if (lMesh->Normals.size())
		{
			XMVECTOR n = XMLoadFloat3(&lMesh->Normals[vi]);
			//n = XMVector3Normalize(n);
			XMStoreFloat3(&vertex.Normal, n);
		}
		if (lMesh->UVs.size())	vertex.UVText = lMesh->UVs[vi];
		if (lMesh->Tangents.size()) vertex.TangentU = lMesh->Tangents[vi];

		meshVertices[vi] = vertex;
		meshIndices[vi] = vi;
	}

	lgeoMesh->IsSkinnedMesh = false; // LOD meshes are not skinned

	submesh.IndexCount = meshVertices.size();
	lgeoMesh->DrawArgs[lInnerRIName] = submesh;

	RenderItem& lNewRI = *m_RenderItems[RIName].get();
	BoundingBox::CreateFromPoints(lNewRI.AABB, lMesh->Vertices.size(), &lMesh->Vertices[0], sizeof(lMesh->Vertices[0]));	
	lNewRI.Geometry = NULL;
	lNewRI.LODGeometry[LODLevel] = lgeoMesh.get();
	lNewRI.LODTrianglesCount[LODLevel] = (UINT) meshIndices.size() / 3;

	if (lMesh->VertexPerPolygon == 3 || lMesh->VertexPerPolygon == 4)
		lNewRI.LODGeometry[LODLevel]->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	else
		assert(0);

	//upload geoMesh to GPU

	Utilit3D::UploadMeshToDefaultBuffer<Mesh, VertexGPU, uint32_t>(lgeoMesh.get(), meshVertices, meshIndices);

	//move geoMeshUp		
	m_objectManager->addMesh(lOuterRIName, lgeoMesh);
	iMesh->WasUploaded = true;
}

template<class T>
inline void FBXFileLoader::build_GeoMeshesWithTypedVertex(fbx_Mesh* iMesh, bool skinnedMesh)
{
	bool lIsSkinnedMesh = skinnedMesh;

	std::vector<T> meshVertices;
	std::vector<uint32_t> meshIndices;
	T vertex = {};

	auto lgeoMesh = std::make_unique<Mesh>();

	fbx_Mesh* lMesh = iMesh;
	SubMesh submesh = {};

	std::string lInnerRIName = lMesh->Name;
	std::string lOuterRIName = m_sceneName + "_" + lInnerRIName;

	lgeoMesh->Name = lInnerRIName;	

	if (lMesh->DoNotDublicateVertices == false)
		meshVertices.resize(lMesh->Indices.size()); /* Vertices count = Indices count,
												because if a Vertex can be the same for several faces, but this Vertex should
												have different information for this faces like Normal, TangentUV*/
	else
		meshVertices.resize(lMesh->Vertices.size());

	meshIndices.resize(lMesh->Indices.size());

	for (int vi = 0; vi < lMesh->Indices.size(); vi++)
	{
		vertex = {};
		vertex.Pos = lMesh->Vertices[lMesh->Indices[vi]];

		if (lMesh->DoNotDublicateVertices == false)
		{
			if (lMesh->Normals.size())
			{
				XMVECTOR n = XMLoadFloat3(&lMesh->Normals[vi]);
				//n = XMVector3Normalize(n);
				XMStoreFloat3(&vertex.Normal, n);
			}
			if (lMesh->UVs.size())	vertex.UVText = lMesh->UVs[vi];
			if (lMesh->Tangents.size()) vertex.TangentU = lMesh->Tangents[vi];

			// write vertex/bone weight information		
			if (lIsSkinnedMesh)
			{
				addSkinnedInfoToVertex(vertex, lMesh, vi);
			}

			// with vertex dublication
			meshVertices[vi] = vertex;
			meshIndices[vi] = vi;
		}
		else
		{
			// without vertex dublication
			int lVertId = lMesh->Indices[vi];
			if (lMesh->UVs.size())	vertex.UVText = lMesh->UVs[lVertId];
			meshVertices[lVertId] = vertex;
			meshIndices[vi] = lVertId;
		}
	}

	lgeoMesh->IsSkinnedMesh = lIsSkinnedMesh; // Does this mesh use at leas one skinned vertex

	submesh.VertextCount= meshVertices.size();
	submesh.IndexCount = meshIndices.size();
	lgeoMesh->DrawArgs[lInnerRIName] = submesh;

	//Create RenderItem for this mesh		
	m_RenderItems[lInnerRIName] = std::make_unique<RenderItem>();
	RenderItem& lNewRI = *m_RenderItems[lInnerRIName].get();
	BoundingBox::CreateFromPoints(lNewRI.AABB, lMesh->Vertices.size(), &lMesh->Vertices[0], sizeof(lMesh->Vertices[0]));
	lNewRI.Name = lOuterRIName;
	lNewRI.Geometry = lgeoMesh.get();
	lNewRI.LODTrianglesCount[0] = (UINT) lMesh->Indices.size() / 3;
	lNewRI.ExcludeFromCulling = iMesh->ExcludeFromCulling;
	lNewRI.ExcludeFromReflection = iMesh->ExcludeFromMirrorReflection;

	if (lMesh->VertexPerPolygon == 3 || lMesh->VertexPerPolygon == 4)
		lNewRI.Geometry->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	else
		assert(0);

	//upload geoMesh to GPU

	Utilit3D::UploadMeshToDefaultBuffer<Mesh, T, uint32_t>(lgeoMesh.get(), meshVertices, meshIndices);

	//move geoMeshUp		
	m_objectManager->addMesh(lOuterRIName, lgeoMesh);
	iMesh->WasUploaded = true;
}

inline void FBXFileLoader::addSkinnedInfoToVertex(VertexExtGPU& vertex, fbx_Mesh* iMesh, int vi)
{
	for (int wi = 0; wi < iMesh->VertexWeightByBoneName[iMesh->Indices[vi]].size(); wi++)
	{
		string lBoneName = iMesh->VertexWeightByBoneName[iMesh->Indices[vi]][wi].first;
		float lBoneWeight = iMesh->VertexWeightByBoneName[iMesh->Indices[vi]][wi].second;
		int lBoneID = m_BonesIDByName[lBoneName].first;
		vertex.BoneIndices[wi] = lBoneID;
		vertex.BoneWeight[wi] = lBoneWeight;
	}
}

inline void FBXFileLoader::addSkinnedInfoToVertex(VertexGPU& vertex, fbx_Mesh* iMesh, int vi)
{
	// Do nothing. We never should be here
}

void FBXFileLoader::build_Animation()
{
	// get animation steck count
	int lAnimationStackCount = m_scene->GetSrcObjectCount<FbxAnimStack>();

	if (lAnimationStackCount == 0) return; 

	//for (int i = lAnimationStackCount-1; i < lAnimationStackCount; i++)
	for (int i = 0; i < lAnimationStackCount; i++)
	{		
		FbxAnimStack* lpAnimStack = FbxCast<FbxAnimStack>(m_scene->GetSrcObject<FbxAnimStack>(i)); //run 3
		add_AnimationStack(lpAnimStack);
	}
}

void FBXFileLoader::add_AnimationStack(FbxAnimStack* animationStack)
{
	m_scene->SetCurrentAnimationStack(animationStack);
	
	string lAnimStackName = animationStack->GetName();
	int lAnimLayerCount = animationStack->GetMemberCount<FbxAnimLayer>();
	assert(lAnimLayerCount);
	FbxAnimLayer* lAnimLayer = animationStack->GetMember<FbxAnimLayer>(0); // we use the first layer only
	
	// Check and add Animation data for Skeleton objects - SkinnedAnimated objects
	for (int i = 0; i < m_rootBones.size(); i++)
	{
		SkinnedData* lSkeleton = &m_skeletonManager->getSkeletonSkinnedAnimated(m_rootBones[i]->Name); // Here Skeleton should be already created
		add_AnimationInfo(lAnimLayer, lSkeleton, m_rootBones[i], lAnimStackName);
		lSkeleton->addAnimationName(lAnimStackName);
	}	

	// Check and add Animation data for Objects without Skeletons (Camera and Lights) - NodeAnimated objects
	{
		// Camera
		for (int i = 0; i < m_cameraNodes.size(); i++)
		{
			SkinnedData* lSkeleton = &m_skeletonManager->getSkeletonNodeAnimated(m_cameraNodes[i]->Name); // Here Skeleton should be already created
			if (add_AnimationInfo(lAnimLayer, lSkeleton, m_cameraNodes[i], lAnimStackName))
				lSkeleton->addAnimationName(lAnimStackName);
		}

		// Lights
		for (int i = 0; i < m_lightsNodes.size(); i++)
		{
			SkinnedData* lSkeleton = &m_skeletonManager->getSkeletonNodeAnimated(m_lightsNodes[i]->Name); // Here Skeleton should be already created
			if (add_AnimationInfo(lAnimLayer, lSkeleton, m_lightsNodes[i], lAnimStackName))
				lSkeleton->addAnimationName(lAnimStackName);
		}
	}
}

bool  FBXFileLoader::add_AnimationInfo(FbxAnimLayer* animationLayer, SkinnedData* skeleton, fbx_TreeBoneNode* bone, string& animationName)
{
	bool lHasAnimation = false;
	BoneAnimation lBoneAnimation = {};	
	BoneData* lBoneData = skeleton->getBone(bone->Name);

	FbxAnimCurve* lAnimCurve = NULL;		
	// we think that all Curves for node have the same first and last Time values, so let's use Translation_X to get it
	lAnimCurve = get_AnimationCurve(bone->Node, animationLayer);	

	if (lAnimCurve != NULL)
	{
		lHasAnimation = true;

		int lKeyCount = lAnimCurve->KeyGetCount();
		FbxTime lFirstKeyTimeValue = lAnimCurve->KeyGetTime(0);
		FbxTime lLastKeyTimeValue = lAnimCurve->KeyGetTime(lKeyCount - 1);

		char lTimeString[256];
		string lsFirstValue = lFirstKeyTimeValue.GetTimeString(lTimeString, FbxUShort(256));
		string lsLastValue = lLastKeyTimeValue.GetTimeString(lTimeString, FbxUShort(256));
		float lfFirstTime = atof(lsFirstValue.c_str());
		float lfLastTime = atof(lsLastValue.c_str());

		FbxTime lFrameTime;
		for (float i = lfFirstTime; i <= lfLastTime; i++)
		{
			float lTime = 1.0f / 30.0f * i;
			lFrameTime.SetSecondDouble(lTime);

			//FbxAMatrix lTrasformation = bone->Node->EvaluateLocalTransform(lFrameTime);
			FbxVector4 lTranslation = bone->Node->EvaluateLocalTranslation(lFrameTime);
			FbxVector4 lScaling = bone->Node->EvaluateLocalScaling(lFrameTime);
			FbxVector4 lRotation = bone->Node->EvaluateLocalRotation(lFrameTime);

			KeyFrame lKeyFrame = {};
			//convertFbxMatrixToFloat4x4(lTrasformation, lKeyFrame.Transform);
			convertFbxVector4ToFloat4(lTranslation, lKeyFrame.Translation);
			convertFbxVector4ToFloat4(lScaling, lKeyFrame.Scale);
			convertFbxVector4ToFloat4(lRotation, lKeyFrame.Rotation);
			lKeyFrame.Rotation.x *= XM_PI / 180;
			lKeyFrame.Rotation.y *= XM_PI / 180;
			lKeyFrame.Rotation.z *= XM_PI / 180;
			lKeyFrame.TimePos = lTime;
			lBoneAnimation.addKeyFrame(lKeyFrame);
		}
		lBoneData->addAnimation(animationName, lBoneAnimation);
	}
	// Get Bind Matrix
	get_BindMatrix(bone->Name, lBoneData->m_bindTransform);

	//Get the same data for children-nodes
	for (int i = 0; i < bone->Childs.size(); i++)
		lHasAnimation |= add_AnimationInfo(animationLayer, skeleton, bone->Childs[i], animationName);

	return lHasAnimation;
}

FbxAnimCurve* FBXFileLoader::get_AnimationCurve(FbxNode* node, FbxAnimLayer* animationLayer)
{
	FbxAnimCurve* lCurve = node->LclTranslation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_X);
	if (lCurve != NULL) return lCurve;
	else
	{
		lCurve = node->LclTranslation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Y);
		if (lCurve != NULL) return lCurve;
		else
		{
			lCurve = node->LclTranslation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Z);
			if (lCurve != NULL) return lCurve;
			else
			{
				lCurve = node->LclRotation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_X);
				if (lCurve != NULL) return lCurve;
				else
				{
					lCurve = node->LclRotation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Y);
					if (lCurve != NULL) return lCurve;
					else
					{
						lCurve = node->LclRotation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Z);
						return lCurve;						
					}
				}
			}
		}
	}
}

void FBXFileLoader::get_BindMatrix(std::string boneName, DirectX::XMFLOAT4X4& m)
{
	int pose_count = m_scene->GetPoseCount();

	for (int i = 0; i < pose_count; i++) // we will look for the Bind Matrix in all Poses, but will leave once we found the first one
	{
		FbxPose* pose = m_scene->GetPose(i);
		std::string pose_name = pose->GetName();

		int lPosecount = pose->GetCount();
		for ( int pose_node_id = 0; pose_node_id < lPosecount; pose_node_id++)
		{
			std::string pose_node_name = pose->GetNodeName(pose_node_id).GetCurrentName();
			if (pose_node_name == boneName)
			{				
				FbxMatrix pose_matrix = pose->GetMatrix(pose_node_id);
				pose_matrix = pose_matrix.Inverse();

				FbxAMatrix bindMatrix;
				FbxVector4 r0 = pose_matrix.GetRow(0);
				FbxVector4 r1 = pose_matrix.GetRow(1);
				FbxVector4 r2 = pose_matrix.GetRow(2);
				FbxVector4 r3 = pose_matrix.GetRow(3);

				bindMatrix.SetRow(0, r0);
				bindMatrix.SetRow(1, r1);
				bindMatrix.SetRow(2, r2);
				bindMatrix.SetRow(3, r3);

				convertFbxMatrixToFloat4x4(bindMatrix, m);
				return;
			}
		}
	}
}

void FBXFileLoader::build_Skeleton()
{
	// Create Skeletons for Skeleton-Animations (Wolf)
	for (int i = 0; i < m_rootBones.size(); i++)
	{
		string lRootBoneName = m_rootBones[i]->Name;
		SkinnedData& lSkeleton = m_skeletonManager->getSkeletonSkinnedAnimated(lRootBoneName); // Here Skeleton will be "created"
		lSkeleton.addRootBone(lRootBoneName);
		add_SkeletonBone(lSkeleton, m_rootBones[i]);
	}

	// Create Skeletons for Node-Animations (Camera, Lights)
	{
		// Camera
		for (int i = 0; i < m_cameraNodes.size(); i++)
		{
			string lCameraName = m_cameraNodes[i]->Name;
			SkinnedData& lSkeleton = m_skeletonManager->getSkeletonNodeAnimated(lCameraName); // Here Skeleton will be "created"
			lSkeleton.addRootBone(lCameraName);			
		}

		// Lights
		for (int i = 0; i < m_lightsNodes.size(); i++)
		{
			string lLightName = m_lightsNodes[i]->Name;
			SkinnedData& lSkeleton = m_skeletonManager->getSkeletonNodeAnimated(lLightName); // Here Skeleton will be "created"
			lSkeleton.addRootBone(lLightName);			
		}
	}
}

void FBXFileLoader::add_SkeletonBone(SkinnedData& skeleton, fbx_TreeBoneNode* parentNode)
{
	for (int i = 0; i < parentNode->Childs.size(); i++)
	{
		skeleton.addBone(parentNode->Name, parentNode->Childs[i]->Name);
		add_SkeletonBone(skeleton, parentNode->Childs[i]);
	}
}

void FBXFileLoader::process_NodeInstances()
{
	/* Look at each NodeInstance and decide how to process it:
			- if this node has a material, move this material up with new ID
			- if this MeshNode, add new Instance to corresponding RenderItem and update material ID, if it has
	*/

	for (int i = 0; i < m_NodeInstances.size(); i++)
	{
		fbx_NodeInstance& lNodeInst = m_NodeInstances[i];

		if (lNodeInst.Materials.size())
		{
			assert(lNodeInst.Materials.size() <= 1); // les's we have only one Material for mesh
			build_Materials(lNodeInst.Materials[0]->Name);
		}

		if (lNodeInst.Nodetype == fbx_NodeType::NT_Mesh)
			add_InstanceToRenderItem(lNodeInst);
	}

	// Move all updated RenderItems to up
	move_RenderItems();
}

void FBXFileLoader::build_Materials(string& pMaterialName)
{
	auto mat_it = m_materials.find(pMaterialName);
	if (mat_it != m_materials.end())
	{
		if (mat_it->second.MatCBIndex == -1) // this material was not moved up yet
		{
			mat_it->second.MatCBIndex = getNextMaterialID();
			std::unique_ptr<MaterialCPU> lMaterial = std::make_unique<MaterialCPU>();
			lMaterial->Name = (*mat_it).second.Name;
			lMaterial->DiffuseAlbedo = (*mat_it).second.DiffuseAlbedo;

			int DiffuseTextureCount = 0;
			for (int i = 0; i < (*mat_it).second.TexturesNameByType.size(); i++)
			{
				int liTextureType = -1;
				string type = (*mat_it).second.TexturesNameByType[i].first;
				string texture_name = (*mat_it).second.TexturesNameByType[i].second;
				
				if (type == "diffuse")
					liTextureType = DiffuseTextureCount++;
				else if (type == "normal")
					liTextureType = 2;
				else if (type == "specular")
					liTextureType = 3;
				else if (type == "transparencyF")				
					liTextureType = 4;									

				if (DiffuseTextureCount > 1) DiffuseTextureCount = 1; // we support only two Diffuse texture

				int textureID = m_resourceManager->getTexturePathIDByName(texture_name);
				if (liTextureType >= 0)
				{
					lMaterial->TexturesMask |= (1 << liTextureType);
					int index = liTextureType;
					lMaterial->DiffuseColorTextureIDs[index] = textureID;
				}

				lMaterial->IsTransparent = (*mat_it).second.IsTransparent; // TO_DO: Check this: parametr for Material is updated for each texture
				lMaterial->IsTransparencyFactorUsed= (*mat_it).second.IsTransparencyUsed;
			}

			if (mat_it->second.IsWaterV2)
			{		
				// Save object parameters for compute work on Compute Shader 
				lMaterial->WaterV2_Width = (*mat_it).second.WaterV2_Width;
				lMaterial->WaterV2_Height = (*mat_it).second.WaterV2_Height;
				lMaterial->WaterV2_Velocity= (*mat_it).second.WaterV2_Velocity;
				lMaterial->WaterV2_TimeInterval = (*mat_it).second.WaterV2_TimeInterval;
				lMaterial->WaterV2_Viscosity= (*mat_it).second.WaterV2_Viscosity;	
								 
				//Diffuse[0] - Dummy Texture for Highs values, which will be build in ComputeRender
				int lTextureID = m_resourceManager->addDummyTexturePath();
				int index = DiffuseTextureCount++;
				lMaterial->TexturesMask |= (1 << index);				
				lMaterial->DiffuseColorTextureIDs[index] = lTextureID;
				
				//Diffuse[1] - Dummy Texture for Dynamic Cube Map, which will be build in DCMRender and used for Reflection/Refraction 
				lTextureID = m_resourceManager->addDummyTexturePath();
				index = DiffuseTextureCount++;
				lMaterial->TexturesMask |= (1 << index);
				lMaterial->DiffuseColorTextureIDs[index] = lTextureID;

			}
			m_resourceManager->addMaterial(lMaterial);
			m_materialCountForThisCall++;
		}
	}
	else
		assert(0); // we did not find a material in material array, but it should be there			
}

void FBXFileLoader::add_InstanceToRenderItem(const fbx_NodeInstance& nodeRIInstance)
{
	auto ri_it = m_RenderItems.find(nodeRIInstance.MeshName);
	if (ri_it != m_RenderItems.end()) // we should already have a RenderItem, we just should add new Instance to it
	{
		InstanceDataGPU lBaseInstance = {};		
		lBaseInstance.World = nodeRIInstance.GlobalTransformation;

		if (nodeRIInstance.Materials.size())
			lBaseInstance.MaterialIndex = nodeRIInstance.Materials[0]->MatCBIndex;

		ri_it->second->Visable = nodeRIInstance.Visible;
		ri_it->second->isNotIncludeInWorldBB = !nodeRIInstance.Materials[0]->DoesIncludeToWorldBB;
		//define a type for RenderItem, let's use the first Instance for this and his material
		if (ri_it->second->Type == 0)
		{
			if (nodeRIInstance.Materials[0]->IsWater)
				ri_it->second->Type = RenderItemType::RIT_GH; 
			else if (nodeRIInstance.Materials[0]->IsWaterV2)
				ri_it->second->Type = RenderItemType::RIT_CH;
			else if (nodeRIInstance.Materials[0]->IsSky)
				ri_it->second->Type = RenderItemType::RIT_Sky;
			else if (nodeRIInstance.Materials[0]->IsTransparent || nodeRIInstance.Materials[0]->IsTransparencyUsed)
				ri_it->second->Type = RenderItemType::RIT_Transparent;
			else
				ri_it->second->Type = RenderItemType::RIT_Opaque;
		}

		ri_it->second->ExcludeFromReflection |= nodeRIInstance.Materials[0]->ExcludeFromMirrorReflection;

		ri_it->second->Instances.push_back(lBaseInstance);
	}
}

void FBXFileLoader::move_RenderItems()
{
	// move RenderItems up	
	auto begin_it = m_RenderItems.begin();
	for (; begin_it != m_RenderItems.end(); begin_it++)
	{
		if (begin_it->second->Type == RenderItemType::RIT_Opaque)
		{
			if (begin_it->second->Geometry != NULL) // this is not RI with LOD meshes
			{
				if (begin_it->second->Geometry->IsSkinnedMesh)
				{
					m_objectManager->addSkinnedOpaqueObject(begin_it->second);
					continue;
				}				
			}

			m_objectManager->addOpaqueObject(begin_it->second);
		}
		else if (begin_it->second->Type == RenderItemType::RIT_Transparent)
		{
			if (begin_it->second->Geometry != NULL) // this is not RI with LOD meshes
			{
				if (begin_it->second->Geometry->IsSkinnedMesh)
				{
					m_objectManager->addSkinnedNotOpaqueObject(begin_it->second);
					continue;
				}
			}
			
			m_objectManager->addTransparentObject(begin_it->second);
		}
		else if (begin_it->second->Type == RenderItemType::RIT_GH)		
			m_objectManager->addTransparentObjectGH(begin_it->second);		
		else if (begin_it->second->Type == RenderItemType::RIT_CH)
			m_objectManager->addTransparentObjectCH(begin_it->second);
		else if (begin_it->second->Type == RenderItemType::RIT_Sky)		
			m_objectManager->addSky(begin_it->second);		
		else
			assert(0); 		
	}
	
}

int FBXFileLoader::getNextMaterialID()
{
	return  m_materialCountForPrevCalling + m_materialLastAddedID++;
}

void FBXFileLoader::convertFbxMatrixToFloat4x4(FbxAMatrix& fbxm, DirectX::XMFLOAT4X4& m4x4)
{
	FbxVector4 r0 = fbxm.GetRow(0);
	FbxVector4 r1 = fbxm.GetRow(1);
	FbxVector4 r2 = fbxm.GetRow(2);
	FbxVector4 r3 = fbxm.GetRow(3);

	XMMATRIX m = XMMatrixSet(
		r0.mData[0], r0.mData[1], r0.mData[2], r0.mData[3],
		r1.mData[0], r1.mData[1], r1.mData[2], r1.mData[3],
		r2.mData[0], r2.mData[1], r2.mData[2], r2.mData[3],
		r3.mData[0], r3.mData[1], r3.mData[2], r3.mData[3]);

	//m = XMMatrixTranspose(m);
	//m = XMMatrixIdentity();
	XMStoreFloat4x4(&m4x4, m);
}

void FBXFileLoader::convertFbxVector4ToFloat4(FbxVector4& fbxv, DirectX::XMFLOAT4& v)
{
	FbxDouble* lData = fbxv.Buffer();	
	v = XMFLOAT4(lData[0], lData[1], lData[2], lData[3]);
}

// ---------------- FBX functions -----------------------------
void FBXFileLoader::process_node(const FbxNode* pNode)// , bool isLODMesh = false
{
	AutoIncrementer lOutShifter;
	string nodeName = pNode->GetName();
	m_logger->log("[NODE]: " + nodeName, lOutShifter.Shift);
	const FbxNodeAttribute* pNodeAtrib = pNode->GetNodeAttribute();	

	fbx_NodeInstance newNodeInstance = {};	
	newNodeInstance.Visible = pNode->GetVisibility();	
	   	 
	//Collect materials for this Node
	process_node_getMaterial(pNode, newNodeInstance);

	if (pNodeAtrib != NULL)
	{
		FbxNodeAttribute::EType type = pNodeAtrib->GetAttributeType();
		switch (type)
		{
		case fbxsdk::FbxNodeAttribute::eUnknown:			
			break;
		case fbxsdk::FbxNodeAttribute::eNull:
			//if it is NULL, it can be a "root" node for Skeleton, let's check it		
			process_node(pNode->GetChild(0));
			break;
		case fbxsdk::FbxNodeAttribute::eMarker:
			break;
		case fbxsdk::FbxNodeAttribute::eSkeleton:
		{
			fbx_TreeBoneNode* lRootBone = new fbx_TreeBoneNode();
			FbxNode* parentNode = const_cast<FbxNode*>(pNode->GetParent());
			std::string name = parentNode->GetName();
			lRootBone->Name = name;
			lRootBone->Node = parentNode;
			m_BonesIDByName[name].first = m_BoneGlobalID++; /*TO_DO: here can be wrong as several Skeleton can have same name for Bone, check it*/
			m_BonesIDByName[name].second = lRootBone;

			process_Skeleton(pNode, lRootBone);
			m_rootBones.push_back(lRootBone);
		}
		break;
		case fbxsdk::FbxNodeAttribute::eMesh:
		{
			// We use the first material of a node to identify which kind this Mesh is: simple mesh or mesh for Tessalation 
			bool lSimpleMesh = true;
			bool lWater2Mesh = false;
			fbx_Material* lMaterial=nullptr;
			{
				int mCount = pNode->GetMaterialCount();
				if (mCount) {

					std::string lMaterialName = pNode->GetMaterial(0)->GetName();
					if (m_materials.find(lMaterialName) != m_materials.end())
						lMaterial = &m_materials[lMaterialName];
					
					if (lMaterial != nullptr)
					{
						if (lMaterial->IsWater)
							lSimpleMesh = false;
						if (lMaterial->IsWaterV2)
							lWater2Mesh = true;
					}
				}
			}

			if (lWater2Mesh)
				newNodeInstance.MeshName = create_WaterV2Mesh(lMaterial, pNodeAtrib);
			else
				newNodeInstance.MeshName = process_mesh(pNodeAtrib, !lSimpleMesh);

			FbxNode* lNode2 = const_cast<FbxNode*>(pNode);			

			FbxAMatrix lGlobalTransform = lNode2->EvaluateGlobalTransform();			

			if (lWater2Mesh) // For WaterV2 object we do not need Scale component, we use numerical parametrs for size
			{
				FbxVector4 lRotateV = lNode2->EvaluateLocalRotation();
				FbxVector4 lTranslateV = lNode2->EvaluateLocalTranslation();
				FbxVector4 lScalingV(1.0f, 1.0f, 1.0f);
				FbxAMatrix lResult(lTranslateV, lRotateV, lScalingV);				
				lGlobalTransform = lResult;
			}

			convertFbxMatrixToFloat4x4(lGlobalTransform, newNodeInstance.GlobalTransformation);			
			
			newNodeInstance.Nodetype = NT_Mesh;

			int lChildCount = pNode->GetChildCount();			
			// Mesh can have sub-meshes"
			for (int i = 0; i < lChildCount; i++)
				process_node(pNode->GetChild(i));
		}
		break;
		
		case fbxsdk::FbxNodeAttribute::eCamera:
		{
			FbxNode* lNode2 = const_cast<FbxNode*>(pNode);
			string name = lNode2->GetName();			
			
			process_camera(pNodeAtrib, lNode2);

			fbx_TreeBoneNode* lCameraBone = new fbx_TreeBoneNode();			
			
			lCameraBone->Name = name;
			lCameraBone->Node = lNode2;
			m_cameraNodes.push_back(lCameraBone);
		}

			break;		
		case fbxsdk::FbxNodeAttribute::eLight:
		{
			FbxNode* lNode2 = const_cast<FbxNode*>(pNode);
			string name = lNode2->GetName();

			process_light(pNodeAtrib, lNode2);

			fbx_TreeBoneNode* lLightBone = new fbx_TreeBoneNode();

			lLightBone->Name = name;
			lLightBone->Node = lNode2;
			m_lightsNodes.push_back(lLightBone);
		}
			break;				
		case fbxsdk::FbxNodeAttribute::eLODGroup:
		{
			process_node_LOD(pNode);
		}
			break;

		default:
			break;
		}
	}

	m_NodeInstances.push_back(newNodeInstance);	
}

void FBXFileLoader::process_node_LOD(const FbxNode* pNode)
{
	/*
		Input:
			(FbxNode* pNode) - LOD group with three LOD childs: LOD0, LOD1, LOD2

		Output:
		- A only one Node_Instance for the first child LOD0. LOD1 and LOD2 no need Node instances
		- Materials (only one actually) for the first child LOD0. LOD1 and LOD2 will 'use' materials for LOD0
	*/
	AutoIncrementer lOutShifter;

	assert(pNode->GetChildCount() == 3);

	string nodeName = pNode->GetName();
	m_logger->log("[NODE LOD GROUP]: " + nodeName, lOutShifter.Shift);
	
	bool lDoesGroupExist = (m_LODGroupByName.find(nodeName) != m_LODGroupByName.end());
	assert(!lDoesGroupExist);

	//vector<fbx_Mesh*> lLODGroup= m_LODGroupByName[nodeName]; // create new LOD Group
	m_LODGroupByName[nodeName].resize(3);; // create new LOD Group with three child	
	
	const FbxNode* pChild = nullptr;

	// The first child - LOD0
	{
		pChild = pNode->GetChild(0);
		const FbxNodeAttribute* pNodeAtrib = pChild->GetNodeAttribute();

		string childName = pChild->GetName();

		fbx_NodeInstance newNodeInstance = {};
		newNodeInstance.MeshName = pNodeAtrib->GetName(); 
		newNodeInstance.Visible = pChild->GetVisibility();

		//Collect materials for the first child
		process_node_getMaterial(pChild, newNodeInstance);

		if (pNodeAtrib != NULL)
		{
			FbxNodeAttribute::EType type = pNodeAtrib->GetAttributeType();
			
			assert(type == fbxsdk::FbxNodeAttribute::eMesh);
			
				newNodeInstance.MeshName  = process_mesh(pNodeAtrib, false, 0, &nodeName); // Name of the first Mesh = LOD group name
				
				FbxNode* lNode2 = const_cast<FbxNode*>(pNode);

				FbxAMatrix lGlobalTransform = lNode2->EvaluateGlobalTransform();
				convertFbxMatrixToFloat4x4(lGlobalTransform, newNodeInstance.GlobalTransformation);				

				newNodeInstance.Nodetype = NT_Mesh;						
		}

		m_NodeInstances.push_back(newNodeInstance);
	}

	// The second child - LOD1
	{
		pChild = pNode->GetChild(1);
		const FbxNodeAttribute* pNodeAtrib = pChild->GetNodeAttribute();		

		if (pNodeAtrib != NULL)
		{
			FbxNodeAttribute::EType type = pNodeAtrib->GetAttributeType();

			assert(type == fbxsdk::FbxNodeAttribute::eMesh);

			process_mesh(pNodeAtrib, false, 1, &nodeName); // LOD1			
		}		
	}

	// The third child - LOD2
	{
		pChild = pNode->GetChild(2);
		const FbxNodeAttribute* pNodeAtrib = pChild->GetNodeAttribute();

		if (pNodeAtrib != NULL)
		{
			FbxNodeAttribute::EType type = pNodeAtrib->GetAttributeType();

			assert(type == fbxsdk::FbxNodeAttribute::eMesh);

			process_mesh(pNodeAtrib, false, 2, &nodeName); // LOD2			
		}
	}	
}

void FBXFileLoader::process_node_getMaterial(const FbxNode* pNode, fbx_NodeInstance& newNodeInstance)
{

	AutoIncrementer lOutShifter;
	fbx_Material lMaterial = {};
	int mCount = pNode->GetMaterialCount();
	for (int i = 0; i < mCount; i++)
	{
		FbxSurfaceMaterial* materialSuface = pNode->GetMaterial(i);
		lMaterial.Name = materialSuface->GetName();

		m_logger->log("[MATERIAL]: " + lMaterial.Name, lOutShifter.Shift);

		if (m_materials.find(lMaterial.Name) == m_materials.end()) //if we do not have this material, lets add it
		{
			FbxSurfacePhong* lphongMaterial = NULL;
			FbxSurfaceLambert* llambertMaterial = NULL;
			if (materialSuface->ShadingModel.Get() == "Phong")
			{
				lphongMaterial = static_cast<FbxSurfacePhong*>(materialSuface);

				//Check Custom Property
				FbxProperty lcustomProperty = lphongMaterial->FindProperty("myCustomProperty");
				if (lcustomProperty != NULL)
				{
					int lValue = lcustomProperty.Get<int>();
					if (lValue == 1) // Water. Uses Tesselation stage.
						lMaterial.IsWater = true;
					else if (lValue == 2)
						lMaterial.IsSky = true;
					else if (lValue == 3) // Water Plane. Uses Compute shader
					{
						lMaterial.IsWaterV2 = true;

						float lValue = 0;

						// WaterPlane_Width
						lcustomProperty = lphongMaterial->FindProperty("WaterPlane_Width");
						if (lcustomProperty != NULL)
							lValue = lcustomProperty.Get<float>();
						else
							lValue = 1.0f;
						lMaterial.WaterV2_Width = lValue;
						
						// WaterPlane_Height
						lcustomProperty = lphongMaterial->FindProperty("WaterPlane_Height");
						if (lcustomProperty != NULL)
							lValue = lcustomProperty.Get<float>();
						else
							lValue = 1.0f;
						lMaterial.WaterV2_Height= lValue;

						// WaterPlane_BlocksCountX
						lcustomProperty = lphongMaterial->FindProperty("WaterPlane_BlocksCountX");
						if (lcustomProperty != NULL)
							lValue = lcustomProperty.Get<float>();
						else
							lValue = 1.0f;
						lMaterial.WaterV2_BlocksCountX= lValue;

						// WaterPlane_Velocity
						lcustomProperty = lphongMaterial->FindProperty("WaterPlane_Velocity");
						if (lcustomProperty != NULL)
							lValue = lcustomProperty.Get<float>();
						else
							lValue = 2.0f;
						lMaterial.WaterV2_Velocity= lValue;

						// WaterPlane_TimeInterval
						lcustomProperty = lphongMaterial->FindProperty("WaterPlane_TimeInterval");
						if (lcustomProperty != NULL)
							lValue = lcustomProperty.Get<float>();
						else
							lValue = 0.03f;
						lMaterial.WaterV2_TimeInterval= lValue;

						// WaterPlane_Viscosity
						lcustomProperty = lphongMaterial->FindProperty("WaterPlane_Viscosity");
						if (lcustomProperty != NULL)
							lValue = lcustomProperty.Get<float>();
						else
							lValue = 0.02f;
						lMaterial.WaterV2_Viscosity= lValue;
					}						
				}

				lMaterial.DoesIncludeToWorldBB = true;
				lcustomProperty = lphongMaterial->FindProperty("myCustomProperty_IncludeToWorldBB");
				if (lcustomProperty != NULL)
				{
					int lValue = lcustomProperty.Get<int>();
					if (lValue == 0)
						lMaterial.DoesIncludeToWorldBB = false;
				}

				lMaterial.ExcludeFromMirrorReflection = false;
				lcustomProperty = lphongMaterial->FindProperty("ExcludeFromReflection");
				if (lcustomProperty != NULL)
				{
					bool lValue = lcustomProperty.Get<bool>();
					lMaterial.ExcludeFromMirrorReflection = lValue;
				}				
				
				// Get Diffuse data
				{
					FbxDouble* lData = lphongMaterial->Diffuse.Get().Buffer();
					float f1 = lData[0];
					float f2 = lData[1];
					float f3 = lData[2];
					float f4 = lphongMaterial->AmbientFactor;

					lMaterial.DiffuseAlbedo = XMFLOAT4(f1, f2, f3, f4);
					read_texture_data(&lMaterial, &lphongMaterial->Diffuse, "diffuse");
				}

				// Get Specular data
				{
					FbxDouble* lData = lphongMaterial->Specular.Get().Buffer();
					float f1 = lData[0];
					float f2 = lData[1];
					float f3 = lData[2];
					float f4 = lphongMaterial->SpecularFactor;

					lMaterial.Specular = XMFLOAT4(f1, f2, f3, f4);
					read_texture_data(&lMaterial, &lphongMaterial->Specular, "specular");
				}

				// Get Normal data
				{
					read_texture_data(&lMaterial, &lphongMaterial->NormalMap, "normal");
				}

				// Get Transparency data
				{
					/*WARNING we do not use Transparent factor now, only Transparency texture;*/
					FbxDouble* lData = lphongMaterial->TransparentColor.Get().Buffer();
					float f1 = lData[0];
					float f2 = lData[1];
					float f3 = lData[2];
					float f4 = lphongMaterial->TransparencyFactor;

					lMaterial.IsTransparent = (f4 != 1.0f);
					lMaterial.IsTransparent = ((f1 == f2 == f3) != 1.0f);
					//lMaterial.IsTransparent = false;// we do not process Transparent factor now;

					lMaterial.TransparencyColor = XMFLOAT4(f1, f2, f3, f4);
					//read_texture_data(&lMaterial, &lphongMaterial->TransparentColor, "transparentC");						
					if (read_texture_data(&lMaterial, &lphongMaterial->TransparencyFactor, "transparencyF"))
						lMaterial.IsTransparencyUsed = true;
				}
			}
			else if (materialSuface->ShadingModel.Get() == "Lambert")
				llambertMaterial = static_cast<FbxSurfaceLambert*>(materialSuface);

			m_materials[lMaterial.Name] = lMaterial;
		}

		newNodeInstance.Materials.push_back(&m_materials[lMaterial.Name]);
	}	
}

string FBXFileLoader::process_mesh(const FbxNodeAttribute* pNodeAtribute, bool meshForTesselation, 
	int LOD_level, std::string* groupName)
{
	AutoIncrementer lOutShifter;
	const FbxMesh* lpcMesh = static_cast<const FbxMesh*>(pNodeAtribute);
	FbxMesh* lpMesh = const_cast<FbxMesh*>(lpcMesh);
	
	string name = lpcMesh->GetName();		

	FbxNode* lParentNode = pNodeAtribute->GetNode();	
	
	FbxProperty lcustomProperty = lParentNode->FindProperty("LODParentMesh");
	if (lcustomProperty != NULL)
	{
		FbxString lValue = lcustomProperty.Get<FbxString>();
		string value(lValue.Buffer());
		return value; // we do not need process mesh for "Instances" anyway, only for meshes from LOD group
	}
	if (LOD_level == 0)
		name = *groupName; // for the first mesh from LOD group we use name of this group, because Blender can change name for mesh, we cannot trust it
	
	if (m_meshesByName.find(name) != m_meshesByName.end()) return name;

	m_logger->log("[MESH]: " + name, lOutShifter.Shift);

	FbxVector4* vertexData = lpcMesh->GetControlPoints();
	int vertex_count = lpcMesh->GetControlPointsCount();//get count unique vertices	
	int polygon_count = lpcMesh->GetPolygonCount();// how many polygons we have
	
	auto lmesh = std::make_unique<fbx_Mesh>();
	lmesh->Name = name;
	lmesh->WasUploaded = false;
	lmesh->ExcludeFromCulling = false;
	
	// Should be this Mesh (with all Instances) excluded from Frustum culling in future? (for the Sky and the Land meshes)	
	lcustomProperty = pNodeAtribute->FindProperty("ExcludeFromCulling");
	if (lcustomProperty != NULL)
	{
		bool lValue = lcustomProperty.Get<bool>();
		lmesh->ExcludeFromCulling = lValue;
	}

	// Should be this Mesh (with all Instances) excluded from mirror reflection	
	lcustomProperty = pNodeAtribute->FindProperty("ExcludeFromReflection");
	if (lcustomProperty != NULL)
	{
		bool lValue = lcustomProperty.Get<bool>();
		lmesh->ExcludeFromMirrorReflection= lValue;
	}

	// Copy All Vertices
	for (int i = 0; i < vertex_count; i++, vertexData++)
		lmesh->Vertices.push_back(XMFLOAT3(vertexData->mData[0], vertexData->mData[1], vertexData->mData[2]));

	// Prepare for UV
	const FbxLayerElementUV* UVLayer = lpcMesh->GetElementUV();
	if (UVLayer)
	{
		FbxGeometryElement::EMappingMode mappingMode = UVLayer->GetMappingMode();
		FbxGeometryElement::EReferenceMode referenceMode = UVLayer->GetReferenceMode();

		bool lWeAreOkWithUVLayerMapping = mappingMode == FbxGeometryElement::EMappingMode::eByPolygonVertex &&
			referenceMode == FbxGeometryElement::EReferenceMode::eIndexToDirect;

		assert(lWeAreOkWithUVLayerMapping);
	}

	// Prepare data for Normals
	const FbxLayerElementNormal* normalLayer = lpcMesh->GetElementNormal();
	vector<XMFLOAT3> averageNormal(lmesh->Vertices.size());
	if (normalLayer)
	{
		FbxGeometryElement::EMappingMode mappingMode = normalLayer->GetMappingMode();
		FbxGeometryElement::EReferenceMode referenceMode = normalLayer->GetReferenceMode();

		bool lWeAreOkWithNormalMappingMode = mappingMode == FbxGeometryElement::EMappingMode::eByPolygonVertex &&
			referenceMode == FbxGeometryElement::EReferenceMode::eDirect;

		assert(lWeAreOkWithNormalMappingMode);

		int normalCount = normalLayer->GetDirectArray().GetCount();

		// We should sum all normals for the same vertex and normalize it later
		int* indicesData = lpcMesh->GetPolygonVertices(); //get a point to Indices data
		for (int i = 0; i < normalCount; i++)
		{
			FbxVector4 normal = normalLayer->GetDirectArray().GetAt(i);
			XMFLOAT3 fnormal = XMFLOAT3(normal.mData[0], normal.mData[1], normal.mData[2]);
			int vertexID = *(indicesData++);
			XMVECTOR n = XMLoadFloat3(&averageNormal[vertexID]);
			XMVECTOR n0 = XMLoadFloat3(&fnormal);
			n = n + n0;
			XMStoreFloat3(&averageNormal[vertexID], n);
		}
	}
	
	const FbxLayerElementTangent* tangentLayer = lpcMesh->GetElementTangent();
	if (tangentLayer)
	{
		FbxGeometryElement::EMappingMode mappingMode = tangentLayer->GetMappingMode();
		FbxGeometryElement::EReferenceMode referenceMode = tangentLayer->GetReferenceMode();

		bool lWeAreOkWithTangentLayerMapping = mappingMode == FbxGeometryElement::EMappingMode::eByPolygonVertex &&
			referenceMode == FbxGeometryElement::EReferenceMode::eDirect;

		assert(lWeAreOkWithTangentLayerMapping);
	}

	int lVertexID = 0;
	// Copy All Indices, but we "triangulate" a polygon if PolygonSize = 4
	for (int pi = 0; pi < polygon_count; pi++)
	{		
		
		int polygon_size = lpcMesh->GetPolygonSize(pi); // how many vertices this polygon uses		
		//assert(polygon_size == 3 || polygon_size == 4);			   		

		lmesh->VertexPerPolygon = polygon_size;

		if (polygon_size == 3)
		{
			if (meshForTesselation)
				assert(!meshForTesselation); //we do not process Triangles for Tesselation

			int i0 = lpcMesh->GetPolygonVertex(pi, 0);
			int i1 = lpcMesh->GetPolygonVertex(pi, 1);
			int i2 = lpcMesh->GetPolygonVertex(pi, 2);			

			lmesh->Indices.push_back(i0);
			lmesh->Indices.push_back(i1);
			lmesh->Indices.push_back(i2);

			// UV
			if (UVLayer)
			{
				int lUVID0 = lpMesh->GetTextureUVIndex(pi, 0);
				int lUVID1 = lpMesh->GetTextureUVIndex(pi, 1);
				int lUVID2 = lpMesh->GetTextureUVIndex(pi, 2);

				FbxVector2 uv0 = UVLayer->GetDirectArray().GetAt(lUVID0);
				FbxVector2 uv1 = UVLayer->GetDirectArray().GetAt(lUVID1);
				FbxVector2 uv2 = UVLayer->GetDirectArray().GetAt(lUVID2);

				lmesh->UVs.push_back(XMFLOAT2(uv0.mData[0], 1.0f - uv0.mData[1]));
				lmesh->UVs.push_back(XMFLOAT2(uv1.mData[0], 1.0f - uv1.mData[1]));
				lmesh->UVs.push_back(XMFLOAT2(uv2.mData[0], 1.0f - uv2.mData[1]));
			}

			// Normal
			if (normalLayer)
			{

#ifdef NORMALMODE
				FbxVector4 normalv0 = normalLayer->GetDirectArray().GetAt(lVertexID + 0);
				FbxVector4 normalv1 = normalLayer->GetDirectArray().GetAt(lVertexID + 1);
				FbxVector4 normalv2 = normalLayer->GetDirectArray().GetAt(lVertexID + 2);

				XMFLOAT3 normal0 = XMFLOAT3(normalv0.mData[0], normalv0.mData[1], normalv0.mData[2]);
				XMFLOAT3 normal1 = XMFLOAT3(normalv1.mData[0], normalv1.mData[1], normalv1.mData[2]);
				XMFLOAT3 normal2 = XMFLOAT3(normalv2.mData[0], normalv2.mData[1], normalv2.mData[2]);
#else
				XMFLOAT3 normal0 = averageNormal[i0];
				XMFLOAT3 normal1 = averageNormal[i1];
				XMFLOAT3 normal2 = averageNormal[i2];
#endif // NORMALMODE

				lmesh->Normals.push_back(normal0);
				lmesh->Normals.push_back(normal1);
				lmesh->Normals.push_back(normal2);
			}

			// Tangent
			if (tangentLayer)
			{				
				FbxVector4 tanV0 = tangentLayer->GetDirectArray().GetAt(lVertexID + 0);
				FbxVector4 tanV1 = tangentLayer->GetDirectArray().GetAt(lVertexID + 1);
				FbxVector4 tanV2 = tangentLayer->GetDirectArray().GetAt(lVertexID + 2);

				XMFLOAT3 tangent0(tanV0.mData[0], tanV0.mData[1], tanV0.mData[2]);
				XMFLOAT3 tangent1(tanV1.mData[0], tanV1.mData[1], tanV1.mData[2]);
				XMFLOAT3 tangent2(tanV2.mData[0], tanV2.mData[1], tanV2.mData[2]);

				lmesh->Tangents.push_back(tangent0);
				lmesh->Tangents.push_back(tangent1);
				lmesh->Tangents.push_back(tangent2);
			}
			
			lVertexID += 3;
		}
		else if (polygon_size == 4)
		{
			int i0 = lpcMesh->GetPolygonVertex(pi, 0);
			int i1 = lpcMesh->GetPolygonVertex(pi, 1);
			int i2 = lpcMesh->GetPolygonVertex(pi, 2);
			int i3 = lpcMesh->GetPolygonVertex(pi, 3);

			add_quadInfo_to_mesh<int>(lmesh->Indices, i0, i1, i2, i3, meshForTesselation);

			// UV
			if (UVLayer)
			{
				int lUVID0 = lpMesh->GetTextureUVIndex(pi, 0);
				int lUVID1 = lpMesh->GetTextureUVIndex(pi, 1);
				int lUVID2 = lpMesh->GetTextureUVIndex(pi, 2);
				int lUVID3 = lpMesh->GetTextureUVIndex(pi, 3);

				FbxVector2 uv0 = UVLayer->GetDirectArray().GetAt(lUVID0);
				FbxVector2 uv1 = UVLayer->GetDirectArray().GetAt(lUVID1);
				FbxVector2 uv2 = UVLayer->GetDirectArray().GetAt(lUVID2);
				FbxVector2 uv3 = UVLayer->GetDirectArray().GetAt(lUVID3);

				XMFLOAT2 xmuv0 = XMFLOAT2(uv0.mData[0], 1.0f - uv0.mData[1]);
				XMFLOAT2 xmuv1 = XMFLOAT2(uv1.mData[0], 1.0f - uv1.mData[1]);
				XMFLOAT2 xmuv2 = XMFLOAT2(uv2.mData[0], 1.0f - uv2.mData[1]);
				XMFLOAT2 xmuv3 = XMFLOAT2(uv3.mData[0], 1.0f - uv3.mData[1]);

				add_quadInfo_to_mesh<XMFLOAT2>(lmesh->UVs, xmuv0, xmuv1, xmuv2, xmuv3, meshForTesselation);
			}

			// Normal
			if (normalLayer)
			{
#ifdef NORMALMODE
				FbxVector4 normalv0 = normalLayer->GetDirectArray().GetAt(lVertexID + 0);
				FbxVector4 normalv1 = normalLayer->GetDirectArray().GetAt(lVertexID + 1);
				FbxVector4 normalv2 = normalLayer->GetDirectArray().GetAt(lVertexID + 2);
				FbxVector4 normalv3 = normalLayer->GetDirectArray().GetAt(lVertexID + 3);

				XMFLOAT3 normal0 = XMFLOAT3(normalv0.mData[0], normalv0.mData[1], normalv0.mData[2]);
				XMFLOAT3 normal1 = XMFLOAT3(normalv1.mData[0], normalv1.mData[1], normalv1.mData[2]);
				XMFLOAT3 normal2 = XMFLOAT3(normalv2.mData[0], normalv2.mData[1], normalv2.mData[2]);
				XMFLOAT3 normal3 = XMFLOAT3(normalv3.mData[0], normalv3.mData[1], normalv3.mData[2]);
#else
				XMFLOAT3 normal0 = averageNormal[i0];
				XMFLOAT3 normal1 = averageNormal[i1];
				XMFLOAT3 normal2 = averageNormal[i2];
				XMFLOAT3 normal3 = averageNormal[i3];
#endif // NORMALMODE

				add_quadInfo_to_mesh<XMFLOAT3>(lmesh->Normals, normal0, normal1, normal2, normal3, meshForTesselation);
			}

			// Tangent
			if (tangentLayer)
			{				
				FbxVector4 tanV0 = tangentLayer->GetDirectArray().GetAt(lVertexID + 0);
				FbxVector4 tanV1 = tangentLayer->GetDirectArray().GetAt(lVertexID + 1);
				FbxVector4 tanV2 = tangentLayer->GetDirectArray().GetAt(lVertexID + 2);
				FbxVector4 tanV3 = tangentLayer->GetDirectArray().GetAt(lVertexID + 3);

				XMFLOAT3 tangent0(tanV0.mData[0], tanV0.mData[1], tanV0.mData[2]);
				XMFLOAT3 tangent1(tanV1.mData[0], tanV1.mData[1], tanV1.mData[2]);
				XMFLOAT3 tangent2(tanV2.mData[0], tanV2.mData[1], tanV2.mData[2]);
				XMFLOAT3 tangent3(tanV3.mData[0], tanV3.mData[1], tanV3.mData[2]);

				add_quadInfo_to_mesh<XMFLOAT3>(lmesh->Tangents, tangent0, tangent1, tangent2, tangent3, meshForTesselation);
			}

			lVertexID += 4;
		}
		else
		{
			bool PolygonSizeNot_3_or_4 = false;
			assert(PolygonSizeNot_3_or_4);
		}			
	}

	// get Skin Data
	// If Bone X uses vertex Y with weight W, we will map it with VertexWeightByBoneID[Y]=pair{X, W}	
	{		
		int lskinCount = lpcMesh->GetDeformerCount(FbxDeformer::eSkin);

		if (lskinCount)
		{
			lmesh->VertexWeightByBoneName.resize(lmesh->Vertices.size());
			int lCommonIndicesCount = 0;

			for (int s = 0; s < lskinCount; s++)
			{
				FbxSkin* lSkin = (FbxSkin*)lpcMesh->GetDeformer(s, FbxDeformer::eSkin);
				int lClusterCount = lSkin->GetClusterCount();

				for (int clusterID = 0; clusterID < lClusterCount; clusterID++)
				{
					FbxCluster* lCluster = lSkin->GetCluster(clusterID);
					const char* lClusterMode[] = { "Normalize", "Additive", "Total1" };

					string mode = lClusterMode[lCluster->GetLinkMode()];

					string linkName = "myNone";
					if (lCluster->GetLink() != NULL)
					{
						linkName = lCluster->GetLink()->GetName();
					}

					int lIndicesCount = lCluster->GetControlPointIndicesCount();
					int* lpIndicesData = lCluster->GetControlPointIndices();
					double* lpWeightsData = lCluster->GetControlPointWeights();
					int lBoneID;
					for (int vi = 0; vi < lIndicesCount; vi++)
					{
						int vertexID = lpIndicesData[vi];
						float boneWeight = lpWeightsData[vi];
						auto lPair = std::make_pair(linkName, boneWeight);
						lmesh->VertexWeightByBoneName[vertexID].push_back(lPair);
					}
				}
			}
		}
	}

	// add mesh
	m_meshesByName[name] = std::move(lmesh);

	// This mesh can be LOD-mesh, if it is, let's add it to LOD group
	if (LOD_level != -1)	
		m_LODGroupByName[*groupName][LOD_level] = m_meshesByName[name].get();

	return name;
}

std::string FBXFileLoader::create_WaterV2Mesh(fbx_Material* material, const FbxNodeAttribute* pNodeAtribute)
{
	AutoIncrementer lOutShifter;
	const FbxMesh* lpcMesh = static_cast<const FbxMesh*>(pNodeAtribute);
	FbxMesh* lpMesh = const_cast<FbxMesh*>(lpcMesh);

	string name = lpcMesh->GetName();	

	if (m_meshesByName.find(name) != m_meshesByName.end()) assert(0); // we think we do should not have more 1 instances for WaterPlane object
	m_logger->log("[MESH][WaterV2]: " + name, lOutShifter.Shift);
		
	auto lmesh = std::make_unique<fbx_Mesh>();
	lmesh->Name = name;
	lmesh->WasUploaded = false;
	lmesh->ExcludeFromCulling = false;
	lmesh->DoNotDublicateVertices = true;

	float lWidth = material->WaterV2_Width;	
	float ldx = lWidth / material->WaterV2_BlocksCountX;
	int lN = (int) material->WaterV2_BlocksCountX + 1; // count vertices in horizontal (X)
	int lM = (int) material->WaterV2_Height / ldx + 1; // count vertices in vertical (Y)
	float lHeight = (lM-1) * ldx;
	int lVerticesCount = lN * lM;
	int lTrianglesCount = ((lN -1) * (lM - 1))*2;
	int lIndicesCount = lTrianglesCount * 3;

	lmesh->Vertices.resize(lVerticesCount);
	lmesh->UVs.resize(lVerticesCount);
	lmesh->Indices.resize(lIndicesCount);
	lmesh->VertexPerPolygon = 3;

	// Create Vertices
	int lVertID = 0;
	for (int h = 0; h < lM; h++)
	{
		for (int w = 0; w < lN; w++)
		{
			lmesh->Vertices[lVertID] = XMFLOAT3(-lWidth/2 + w*ldx, lHeight / 2 - h * ldx, 0 );
			lmesh->UVs[lVertID++] = XMFLOAT2((float) w / (float)lN, (float)h/ (float)lM);
			//lmesh->UVs[lVertID++] = XMFLOAT2(w, h);
		}
	}	

	// Create Indices
	int lTrianglID = 0;
	bool lVswap = false;
	bool lHswap = true;
	/*
		lVswap and lHswap are needed to build triangles in this way:	
		/\/\/\/\	
		\/\/\/\/
		/\/\/\/\
		\/\/\/\/
		/\/\/\/\
	*/

	for (int ty = 0; ty <(lM - 1); ty++)
	{
		for (int tx = 0; tx < (lN - 1); tx++)
		{
			int i1, i2, i3, i4;
			int vi0, vi1, vi2, vi3;
				
			i1 = (ty * lN) + tx;
			i2 = i1 + 1;
			i3 = i2 + lN;
			i4 = i1 + lN;

			//lHswap = true;
			if (lHswap)
			{
				vi0 = i1;
				vi1 = i2;
				vi2 = i4;
				vi3 = i3;
			}
			else
			{
				vi0 = i4;
				vi1 = i1;
				vi2 = i3;
				vi3 = i2;
			}		
			//in counter-clockwise order
			lmesh->Indices[lTrianglID + 0] = vi0;
			lmesh->Indices[lTrianglID + 1] = vi2;
			lmesh->Indices[lTrianglID + 2] = vi1;			
			lTrianglID += 3;

			lmesh->Indices[lTrianglID + 0] = vi1;
			lmesh->Indices[lTrianglID + 1] = vi2;
			lmesh->Indices[lTrianglID + 2] = vi3;
			lTrianglID += 3;

			lHswap = !lHswap;
		}
		lHswap = lVswap;
		lVswap = !lVswap;
	}

	// add mesh
	m_meshesByName[name] = std::move(lmesh);	

	material->WaterV2_Width = lN; // we do not need a dimensions in meters anymore, we will work with dimensions in Vertex count
	material->WaterV2_Height = lM;

	return name;
}

void FBXFileLoader::process_camera(const FbxNodeAttribute* pNodeAtribute, FbxNode* pNode)
{	
	const FbxCamera* lpcCamera= static_cast<const FbxCamera*>(pNodeAtribute);
	
	FbxDouble3 lCameraPosition = lpcCamera->Position;
	FbxDouble3 lCameraInterestPosition = lpcCamera->InterestPosition;
	
	FbxDouble3 lCameraUpVector= lpcCamera->UpVector;
	float lNearPlane = lpcCamera->NearPlane;
	lNearPlane = 2.36f;
	float lFar= lpcCamera->FarPlane;
	
	std::unique_ptr<Camera> lCamera = std::make_unique<Camera>();
	
	FbxAMatrix lLocalTransform = pNode->EvaluateLocalTransform();
	DirectX::XMFLOAT4X4 lLocalTransformation;
	convertFbxMatrixToFloat4x4(lLocalTransform, lLocalTransformation);
	lCamera->setLocalTransformation(lLocalTransformation);

	DirectX::XMVECTOR lPos = DirectX::XMVectorSet(lCameraPosition.mData[0], lCameraPosition.mData[1], lCameraPosition.mData[2], 1.0f);	
	DirectX::XMVECTOR lTarget = DirectX::XMVectorSet(lCameraInterestPosition.mData[0], lCameraInterestPosition.mData[1], lCameraInterestPosition.mData[2], 1.0f);

	DirectX::XMVECTOR lLook = lTarget - lPos;	
	DirectX::XMVECTOR lUpVector = DirectX::XMVectorSet(lCameraUpVector.mData[0], lCameraUpVector.mData[1], lCameraUpVector.mData[2], 0.0f);
	
	DirectX::XMVECTOR lWordUpVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMVECTOR lRight = XMVector3Normalize(XMVector3Cross(lWordUpVector, lLook));
	DirectX::XMVECTOR lNewLook = XMVector3Cross(lUpVector, lRight);
	DirectX::XMVECTOR lNewLook2 = XMVector3Cross(lRight, lUpVector);

	lPos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);	
	lLook = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	
	XMMATRIX lCameraLcTr = lCamera->getLocalTransformationMatrix();
	lPos = XMVector3Transform(lPos, lCamera->getLocalTransformationMatrix());	
	lLook = XMVector3Normalize(XMVector3TransformNormal(lLook, lCamera->getLocalTransformationMatrix()));
		
	lCamera->lookTo(lPos, lLook, lWordUpVector);

	lCamera->lens->setLens(0.25f*XM_PI, 1.0f, lNearPlane, lFar);	

	m_objectManager->addCamera(lCamera);	
}

void FBXFileLoader::process_light(const FbxNodeAttribute* pNodeAtribute, FbxNode* pNode)
{
	CPULight lnewLight = {};

	const FbxLight* lLight = static_cast<const FbxLight*>(pNodeAtribute);
	string lName = lLight->GetName();
	
	FbxAMatrix lLocalTransform = pNode->EvaluateLocalTransform();
	DirectX::XMFLOAT4X4 lLocalTransformation;
	convertFbxMatrixToFloat4x4(lLocalTransform, lLocalTransformation);

	FbxLight::EType lLightType = lLight->LightType;

	DirectX::XMVECTOR lPos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR lLook = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

	XMMATRIX lLcTr = DirectX::XMLoadFloat4x4(&lLocalTransformation);
	lPos = XMVector3Transform(lPos, lLcTr);	
	//lLook = XMVector3TransformNormal(lLook, lLcTr);
	lLook = XMVector3Normalize(XMVector3TransformNormal(lLook, lLcTr));

	lnewLight.Name = lName;
	FbxDouble3 lColor = lLight->Color.Get();
	float lIntensity = lLight->Intensity/100.0f;
	lnewLight.Strength = XMFLOAT3(lColor[0] * lIntensity, lColor[1] * lIntensity, lColor[2] * lIntensity);
	
	float lOuterAngle = lLight->OuterAngle;
	float lInnerAngle = lLight->InnerAngle;

	XMStoreFloat3(&lnewLight.Position, lPos);
	XMStoreFloat3(&lnewLight.Direction, lLook);
	XMStoreFloat3(&lnewLight.initDirection, lLook);
		
	lnewLight.turnOn = pNode->GetVisibility(); // we use Node visibility to show/hide a Light
	lnewLight.lightType = LightType::Directional;

	if (lLightType == FbxLight::ePoint) // a Lamp
	{
		FbxLight::EDecayType lType = lLight->DecayType;
		float lFallofStart = lLight->DecayStart;
		lnewLight.falloffStart = 0;
		lnewLight.falloffEnd = lFallofStart * 2;
		lnewLight.lightType = LightType::Point;
	}
	else if (lLightType == FbxLight::eSpot) // a Lamp
	{
		FbxLight::EDecayType lType = lLight->DecayType;
		float lFallofStart = lLight->DecayStart;
		lnewLight.falloffStart = 0;
		lnewLight.falloffEnd = lFallofStart * 2;

		float lOuterAngle = lLight->OuterAngle;
		float lInnerAngle = lLight->InnerAngle;
				
		lnewLight.lightType = LightType::Spot;
	}

	m_objectManager->addLight(lnewLight);

}

bool FBXFileLoader::read_texture_data(
	fbx_Material* destMaterial,	FbxProperty* matProperty, std::string textureType)
{	
	AutoIncrementer lOutShifter;
	bool AtleasOneTextureWasAdded = false;
	int srcObjectcount = matProperty->GetSrcObjectCount<FbxTexture>();

	for (int ti = 0; ti < srcObjectcount; ti++)
	{		
		// We do not support layered texture now, so just take the first texture
		FbxTexture* lTexture = matProperty->GetSrcObject<FbxTexture>(ti);
		assert(lTexture);

		string lTextureName = lTexture->GetName();
		FbxFileTexture* lFileTexture = FbxCast<FbxFileTexture>(lTexture);
		assert(lFileTexture);
		string lTexturePath = lFileTexture->GetFileName();

		m_resourceManager->addTexturePathByName(lTextureName, lTexturePath);

		auto ltextureTypeName = std::make_pair(textureType, lTextureName);
		destMaterial->TexturesNameByType.push_back(ltextureTypeName);
		AtleasOneTextureWasAdded = true;
		m_logger->log("Texture("+ textureType +"): "+ lTextureName+"["+ lTexturePath +"]", lOutShifter.Shift -1);
	}

	return AtleasOneTextureWasAdded;
}

void FBXFileLoader::process_Skeleton(const FbxNode* pNode, fbx_TreeBoneNode* parent)
{
	FbxSkeleton* lpSkeleton = (FbxSkeleton*)pNode->GetNodeAttribute();
	string name = lpSkeleton->GetName();

	fbx_TreeBoneNode* currentNode = new fbx_TreeBoneNode();
	currentNode->Name = name;
	currentNode->Node = const_cast<FbxNode*>(pNode);
	parent->Childs.push_back(currentNode);

	m_BonesIDByName[name].first = m_BoneGlobalID++;
	m_BonesIDByName[name].second = currentNode;

	for (int i = 0; i < pNode->GetChildCount(); i++)
		process_Skeleton(pNode->GetChild(i), currentNode);
}

template<class T>
inline void FBXFileLoader::add_quadInfo_to_mesh(std::vector<T>& m, T t0, T t1, T t2, T t3, bool isTesselated)
{
	if (isTesselated)
	{
		m.push_back(t0);
		m.push_back(t1);
		m.push_back(t3);
		m.push_back(t2);
	}
	else
	{
		m.push_back(t0);
		m.push_back(t1);
		m.push_back(t2);

		m.push_back(t0);
		m.push_back(t2);
		m.push_back(t3);
	}
}
