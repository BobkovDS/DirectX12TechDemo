#include "FBXFileLoader.h"
#include "ApplException.h"


using namespace std;
using namespace DirectX;

int FBXFileLoader::m_materialCountForPrevCalling = 0;

FBXFileLoader::FBXFileLoader()
{
	m_sdkManager = FbxManager::Create();
	m_materialLastAddedID = 0;
	m_materialCountForThisCall = 0;
	m_initialized = false;
}


FBXFileLoader::~FBXFileLoader()
{
	if (m_scene != NULL) m_scene->Destroy();
	m_sdkManager->Destroy();
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
	if (!m_initialized) assert(0);

	// Create the IO settings object
	FbxIOSettings* ios = FbxIOSettings::Create(m_sdkManager, IOSROOT);
	m_sdkManager->SetIOSettings(ios);

	//Create an importer using tje SDK manger
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
	createScene();
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

		We build one MeshGeometry (geoMesh), one RenderItem with this geoMesh for each unique Mesh.
		geoMesh.Instances are used for instancing this mesh.

		It contains:
			1) Build all unique geoMeshes and upload it on GPU (build_GeoMeshes())
			2) Build all materials with through numbering for all unique meshes and upload it on GPU. Here (process_NodeInstances/build_Materials())
			3) Update all renderItems with required instances data and move it up (process_NodeInstances()/build_RenderItems)
	*/

	auto mesh_it_begin = m_meshesByName.begin();
	for (; mesh_it_begin != m_meshesByName.end(); mesh_it_begin++)
	{
		bool lIsSkinnedMesh = false;

		std::vector<VertexExtGPU> meshVertices;
		std::vector<uint32_t> meshIndices;

		VertexExtGPU vertex = {};
		auto lgeoMesh = std::make_unique<Mesh>();

		fbx_Mesh* lMesh = mesh_it_begin->second.get();
		SubMesh submesh = {};

		std::string lInnerRIName = lMesh->Name;
		std::string lOuterRIName = m_sceneName + "_" + lInnerRIName;

		lgeoMesh->Name = lInnerRIName;

		meshVertices.resize(lMesh->Indices.size());
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
			vertex.ShapeID = 0;

			// write vertex/bone weight information		
			for (int wi = 0; wi < lMesh->VertexWeightByBoneName[lMesh->Indices[vi]].size(); wi++)
			{
				string lBoneName = lMesh->VertexWeightByBoneName[lMesh->Indices[vi]][wi].first;
				float lBoneWeight = lMesh->VertexWeightByBoneName[lMesh->Indices[vi]][wi].second;
				int lBoneID = m_BonesIDByName[lBoneName].first;
				vertex.BoneIndices[wi] = lBoneID;
				vertex.BoneWeight[wi] = lBoneWeight;
				lIsSkinnedMesh = true;
			}

			meshVertices[vi] = vertex;
			meshIndices[vi] = vi;
		}
		
		lgeoMesh->IsSkinnedMesh = lIsSkinnedMesh; // Does this mesh use at leas one skinned vertex
		
		submesh.IndexCount = meshVertices.size();
		lgeoMesh->DrawArgs[lInnerRIName] = submesh;

		//Create RenderItem for this mesh		
		m_RenderItems[lInnerRIName] = std::make_unique<RenderItem>();
		RenderItem& lNewRI = *m_RenderItems[lInnerRIName].get();
		BoundingBox::CreateFromPoints(lNewRI.AABB, lMesh->Vertices.size(), &lMesh->Vertices[0], sizeof(lMesh->Vertices[0]));
		lNewRI.Name = lOuterRIName;
		lNewRI.Geometry = lgeoMesh.get();

		if (lMesh->VertexPerPolygon == 3 || lMesh->VertexPerPolygon == 4)
			lNewRI.Geometry->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		else
			assert(0);

		//upload geoMesh to GPU
		
		Utilit3D::UploadMeshToDefaultBuffer<Mesh, VertexExtGPU, uint32_t>(lgeoMesh.get(), meshVertices, meshIndices);

		//move geoMeshUp		
	 	m_objectManager->addMesh(lOuterRIName, lgeoMesh);
	}
}

void FBXFileLoader::build_Animation()
{
	// get animation steck count
	int lAnimationStackCount = m_scene->GetSrcObjectCount<FbxAnimStack>();

	if (lAnimationStackCount == 0) return; 
	lAnimationStackCount = 2;//TEST
	for (int i = lAnimationStackCount-1; i < lAnimationStackCount; i++)
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
	
	for (int i = 0; i < m_rootBones.size(); i++)
	{
		SkinnedData* lSkeleton = &m_skeletonManager->getSkeleton(m_rootBones[i]->Name);
		add_AnimationInfo(lAnimLayer, lSkeleton, m_rootBones[i], lAnimStackName);
		lSkeleton->addAnimationName(lAnimStackName);
	}	
}

void  FBXFileLoader::add_AnimationInfo(FbxAnimLayer* animationLayer, SkinnedData* skeleton, fbx_TreeBoneNode* bone, string& animationName)
{
	BoneAnimation lBoneAnimation = {};	
	BoneData* lBoneData = skeleton->getBone(bone->Name);

	FbxAnimCurve* lAnimCurve = NULL;		
	// we think that all Curves for node have the same first and last Time values, so let's use Translation_X to get it
	lAnimCurve = bone->Node->LclTranslation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_X); 
	
	if (lAnimCurve != NULL)
	{
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
	
	//Get Node LocalTransformation data
	//get_LcTransformationData(bone, lBoneData);

	//Get the same data for children-nodes
	for (int i = 0; i < bone->Childs.size(); i++)
		add_AnimationInfo(animationLayer, skeleton, bone->Childs[i], animationName);
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

void FBXFileLoader::get_LcTransformationData(fbx_TreeBoneNode* src_bone, BoneData* dst_bone)
{
	string name = src_bone->Name;
	FbxDouble3 lTranslation = src_bone->Node->LclTranslation;
	FbxDouble3 lScaling= src_bone->Node->LclScaling;
	FbxDouble3 lRotation= src_bone->Node->LclRotation;
	FbxAMatrix lTransformationM = src_bone->Node->EvaluateLocalTransform();

	XMVECTOR lT = XMVectorSet(lTranslation.mData[0], lTranslation.mData[1], lTranslation.mData[2], 0.0f);
	XMVECTOR lS = XMVectorSet(lScaling.mData[0], lScaling.mData[1], lScaling.mData[2], 0.0f);
	XMVECTOR lR = XMVectorSet(lRotation.mData[0], lRotation.mData[1], lRotation.mData[2], 0.0f);

	XMFLOAT4X4 lMlc;
	convertFbxMatrixToFloat4x4(lTransformationM, lMlc);
	XMMATRIX lM = XMLoadFloat4x4(&lMlc);

	XMMATRIX lTm = XMMatrixTranslationFromVector(lT);
	XMMATRIX lSm = XMMatrixScalingFromVector(lS);
	float lAngle = lRotation.mData[0];
	float lInRadians = lAngle * XM_PI / 180;
	XMMATRIX lRxm = XMMatrixRotationX(lInRadians);
	XMMATRIX lRym = XMMatrixRotationX(lRotation.mData[1]);
	XMMATRIX lRzm = XMMatrixRotationX(lRotation.mData[2]);
	//XMMATRIX lRm = lRzm * lRym * lRxm;
	XMMATRIX lRm = lRxm * lRym *lRzm;
	XMMATRIX lRm2 = XMMatrixRotationRollPitchYawFromVector(lR);

	//XMMATRIX lResult = lTm* lRm * lSm;
	XMMATRIX lResult = lSm * lRm * lTm;
	//lResult = XMMatrixTranspose(lResult);

	// evaluating

	float lt = 0;
	FbxTime lFrameTime;
	lFrameTime.SetSecondDouble(lt);
	FbxAMatrix leTrasformation = src_bone->Node->EvaluateLocalTransform(lFrameTime);
	FbxVector4 leTranslation = src_bone->Node->EvaluateLocalTranslation(lFrameTime);
	FbxVector4 leScaling = src_bone->Node->EvaluateLocalScaling(lFrameTime);
	FbxVector4 leRotation = src_bone->Node->EvaluateLocalRotation(lFrameTime);
	XMFLOAT4X4 leTrns4x4;
	XMFLOAT4 leTransl4;
	XMFLOAT4 leScal4;
	XMFLOAT4 leRot4;

	convertFbxMatrixToFloat4x4(leTrasformation, leTrns4x4);
	convertFbxVector4ToFloat4(leTranslation, leTransl4);
	convertFbxVector4ToFloat4(leScaling, leScal4);
	convertFbxVector4ToFloat4(leRotation, leRot4);

	XMMATRIX leTrnsM = XMLoadFloat4x4(&leTrns4x4);
	XMVECTOR leTranslV = XMLoadFloat4(&leTransl4);
	XMVECTOR leScalingV= XMLoadFloat4(&leScal4);
	XMVECTOR leRotatV = XMLoadFloat4(&leRot4);

	XMMATRIX leTranslM = XMMatrixTranslationFromVector(leTranslV);
	XMMATRIX leScalM= XMMatrixScalingFromVector(leScalingV);
	XMMATRIX leRotatM= XMMatrixRotationRollPitchYawFromVector(leRotatV);

	int a = 1;
}

void FBXFileLoader::build_Skeleton()
{
	for (int i = 0; i < m_rootBones.size(); i++)
	{
		string lRootBoneName = m_rootBones[i]->Name;
		SkinnedData& lSkeleton = m_skeletonManager->getSkeleton(lRootBoneName);
		lSkeleton.addRootBone(lRootBoneName);
		add_SkeletonBone(lSkeleton, m_rootBones[i]);
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

				lMaterial->IsTransparent = (*mat_it).second.IsTransparent;
				lMaterial->IsTransparencyFactorUsed= (*mat_it).second.IsTransparencyUsed;
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
		lBaseInstance.World = nodeRIInstance.LocalTransformation;
		if (nodeRIInstance.Materials.size())
			lBaseInstance.MaterialIndex = nodeRIInstance.Materials[0]->MatCBIndex;

		ri_it->second->Visable = nodeRIInstance.Visible;

		//define a type for RenderItem, let's use the first Instance for this and his material
		if (ri_it->second->Type == 0)
		{
			if (nodeRIInstance.Materials[0]->IsTransparent || nodeRIInstance.Materials[0]->IsTransparencyUsed)
				ri_it->second->Type = RenderItemType::RIT_Transparent;
			else
				ri_it->second->Type = RenderItemType::RIT_Opaque;
		}

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
			if (begin_it->second->Geometry->IsSkinnedMesh)
				m_objectManager->addSkinnedOpaqueObject(begin_it->second);
			else
				m_objectManager->addOpaqueObject(begin_it->second);
		}
		else if (begin_it->second->Type == RenderItemType::RIT_Transparent)
		{
			if (begin_it->second->Geometry->IsSkinnedMesh)
				m_objectManager->addSkinnedNotOpaqueObject(begin_it->second);
			else
				m_objectManager->addTransparentObject(begin_it->second);
		}			
		else
			assert(0); 		
	}
	//
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
void FBXFileLoader::process_node(const FbxNode* pNode)
{
	string nodeName = pNode->GetName();
	const FbxNodeAttribute* pNodeAtrib = pNode->GetNodeAttribute();
	int atribCount = pNode->GetNodeAttributeCount();

	fbx_NodeInstance newNodeInstance = {};
	newNodeInstance.MeshName = pNodeAtrib->GetName();
	newNodeInstance.Visible = pNode->GetVisibility();

	FbxNode::EShadingMode lshadingMode = pNode->GetShadingMode();

	//Collect materials for this Node
	{
		fbx_Material lMaterial = {};
		int mCount = pNode->GetMaterialCount();
		for (int i = 0; i < mCount; i++)
		{
			FbxSurfaceMaterial* materialSuface = pNode->GetMaterial(i);
			lMaterial.Name = materialSuface->GetName();
			if (m_materials.find(lMaterial.Name) == m_materials.end()) //if we do not have this material, lets add it
			{
				FbxSurfacePhong* lphongMaterial = NULL;
				FbxSurfaceLambert* llambertMaterial = NULL;
				if (materialSuface->ShadingModel.Get() == "Phong")
				{
					lphongMaterial = static_cast<FbxSurfacePhong*>(materialSuface);
					/*
					int difuseTextureCount = lphongMaterial->Diffuse.GetSrcObjectCount<FbxTexture>();
					int specularTextureCount = lphongMaterial->Specular.GetSrcObjectCount<FbxTexture>();
					int transparentTextureCount = lphongMaterial->TransparentColor.GetSrcObjectCount<FbxTexture>();
					int ambientTextureCount = lphongMaterial->Ambient.GetSrcObjectCount<FbxTexture>();
					int bumpTextureCount = lphongMaterial->Bump.GetSrcObjectCount<FbxTexture>();
					int displTextureCount = lphongMaterial->DisplacementColor.GetSrcObjectCount<FbxTexture>();
					int emmisiveTextureCount = lphongMaterial->Emissive.GetSrcObjectCount<FbxTexture>();
					int reflectionTextureCount = lphongMaterial->Reflection.GetSrcObjectCount<FbxTexture>();
					int vecotorDisplTextureCount = lphongMaterial->VectorDisplacementColor.GetSrcObjectCount<FbxTexture>();
					int transparencyFactorTextureCount = lphongMaterial->TransparencyFactor.GetSrcObjectCount<FbxTexture>();
					*/
					
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
		
			process_mesh(pNodeAtrib);
					   
			FbxNode* lNode2 = const_cast<FbxNode*>(pNode);
			string name = lNode2->GetName();
			FbxLODGroup* lLODFr = lNode2->GetLodGroup();

			FbxAMatrix lGlobalTransform = lNode2->EvaluateGlobalTransform();
			FbxAMatrix lLocalTransform = lNode2->EvaluateGlobalTransform();

			convertFbxMatrixToFloat4x4(lGlobalTransform, newNodeInstance.GlobalTransformation);
			convertFbxMatrixToFloat4x4(lLocalTransform, newNodeInstance.LocalTransformation);
			newNodeInstance.Nodetype = NT_Mesh;

			int lChildCount = pNode->GetChildCount();

			// Mesh can have sub-meshes"
			for (int i = 0; i < lChildCount; i++)
				process_node(pNode->GetChild(i));
		}
		break;
		
		case fbxsdk::FbxNodeAttribute::eCamera:
			break;		
		case fbxsdk::FbxNodeAttribute::eLight:
			break;				
		case fbxsdk::FbxNodeAttribute::eLODGroup:
			break;		
		default:
			break;
		}
	}

	m_NodeInstances.push_back(newNodeInstance);
}

void FBXFileLoader::process_mesh(const FbxNodeAttribute* pNodeAtribute)
{
	const FbxMesh* lpcMesh = static_cast<const FbxMesh*>(pNodeAtribute);
	FbxMesh* lpMesh = const_cast<FbxMesh*>(lpcMesh);

	string name = lpcMesh->GetName();
		
	if (m_meshesByName.find(name) != m_meshesByName.end()) return;

	FbxVector4* vertexData = lpcMesh->GetControlPoints();
	int vertex_count = lpcMesh->GetControlPointsCount();//get count unique vertices	
	int polygon_count = lpcMesh->GetPolygonCount();// how many polygons we have
	
	auto lmesh = std::make_unique<fbx_Mesh>();
	lmesh->Name = name;

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

		//// We should sum all normals for the same vertex and normalize it later
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
				FbxVector4 normalv0 = normalLayer->GetDirectArray().GetAt(lVertexID + 0);
				FbxVector4 normalv1 = normalLayer->GetDirectArray().GetAt(lVertexID + 1);
				FbxVector4 normalv2 = normalLayer->GetDirectArray().GetAt(lVertexID + 2);
				
				XMFLOAT3 normal0 = XMFLOAT3(normalv0.mData[0], normalv0.mData[1], normalv0.mData[2]);
				XMFLOAT3 normal1 = XMFLOAT3(normalv1.mData[0], normalv1.mData[1], normalv1.mData[2]);
				XMFLOAT3 normal2 = XMFLOAT3(normalv2.mData[0], normalv2.mData[1], normalv2.mData[2]);

				/*XMFLOAT3 normal0 = averageNormal[i0];
				XMFLOAT3 normal1 = averageNormal[i1];
				XMFLOAT3 normal2 = averageNormal[i2];*/				

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

			lmesh->Indices.push_back(i0);
			lmesh->Indices.push_back(i1);
			lmesh->Indices.push_back(i2);

			lmesh->Indices.push_back(i0);
			lmesh->Indices.push_back(i2);
			lmesh->Indices.push_back(i3);

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

				lmesh->UVs.push_back(XMFLOAT2(uv0.mData[0], 1.0f - uv0.mData[1]));
				lmesh->UVs.push_back(XMFLOAT2(uv1.mData[0], 1.0f - uv1.mData[1]));
				lmesh->UVs.push_back(XMFLOAT2(uv2.mData[0], 1.0f - uv2.mData[1]));

				lmesh->UVs.push_back(XMFLOAT2(uv0.mData[0], 1.0f - uv0.mData[1]));
				lmesh->UVs.push_back(XMFLOAT2(uv2.mData[0], 1.0f - uv2.mData[1]));
				lmesh->UVs.push_back(XMFLOAT2(uv3.mData[0], 1.0f - uv3.mData[1]));
			}

			// Normal
			if (normalLayer)
			{
				FbxVector4 normalv0 = normalLayer->GetDirectArray().GetAt(lVertexID + 0);
				FbxVector4 normalv1 = normalLayer->GetDirectArray().GetAt(lVertexID + 1);
				FbxVector4 normalv2 = normalLayer->GetDirectArray().GetAt(lVertexID + 2);
				FbxVector4 normalv3 = normalLayer->GetDirectArray().GetAt(lVertexID + 2);

				XMFLOAT3 normal0 = XMFLOAT3(normalv0.mData[0], normalv0.mData[1], normalv0.mData[2]);
				XMFLOAT3 normal1 = XMFLOAT3(normalv1.mData[0], normalv1.mData[1], normalv1.mData[2]);
				XMFLOAT3 normal2 = XMFLOAT3(normalv2.mData[0], normalv2.mData[1], normalv2.mData[2]);
				XMFLOAT3 normal3 = XMFLOAT3(normalv3.mData[0], normalv3.mData[1], normalv3.mData[2]);

				/*XMFLOAT3 normal0 = averageNormal[i0];
				XMFLOAT3 normal1 = averageNormal[i1];
				XMFLOAT3 normal2 = averageNormal[i2];
				XMFLOAT3 normal3 = averageNormal[i3];*/

				lmesh->Normals.push_back(normal0);
				lmesh->Normals.push_back(normal1);
				lmesh->Normals.push_back(normal2);

				lmesh->Normals.push_back(normal0);
				lmesh->Normals.push_back(normal2);
				lmesh->Normals.push_back(normal3);
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

				lmesh->Tangents.push_back(tangent0);
				lmesh->Tangents.push_back(tangent1);
				lmesh->Tangents.push_back(tangent2);

				lmesh->Tangents.push_back(tangent0);
				lmesh->Tangents.push_back(tangent2);
				lmesh->Tangents.push_back(tangent3);
			}

			lVertexID += 4;
		}
		else
		{
			bool PolygonSizeNot_3_or_4 = false;
			assert(PolygonSizeNot_3_or_4);
		}
			
	}

	// get Material ID for mesh/polygons
	{
		int count = lpcMesh->GetElementMaterialCount(); //just to know. now we will use the 0 material;
		const FbxLayerElementMaterial* materialLayer = lpcMesh->GetElementMaterial();
		FbxGeometryElement::EMappingMode mappingMode = materialLayer->GetMappingMode();

		if (mappingMode == FbxGeometryElement::EMappingMode::eAllSame) // the same Material is used for all mesh
		{
			lmesh->Materials.push_back(materialLayer->GetIndexArray()[0]);
		}
		else if (mappingMode == FbxGeometryElement::EMappingMode::eByPolygon) // each polygon can have own material
		{
			bool MaterialByPolygonIsUsed = false;
			assert(MaterialByPolygonIsUsed); // to catch when we have this variant
		}
	}	

	// get Skin Data
	// If Bone X uses vertex Y with weight W, we will map it with VertexWeightByBoneID[Y]=pair{X, W}
	lmesh->VertexWeightByBoneName.resize(lmesh->Vertices.size());
	{
		int lCommonIndicesCount = 0;
		int lskinCount = lpcMesh->GetDeformerCount(FbxDeformer::eSkin);

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
		int a = 1;
	}

	// add mesh
	m_meshesByName[name] = std::move(lmesh);
}

bool FBXFileLoader::read_texture_data(
	fbx_Material* destMaterial,	FbxProperty* matProperty, std::string textureType)
{	
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