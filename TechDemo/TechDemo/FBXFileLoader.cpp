#include "FBXFileLoader.h"
#include "ApplException.h"


using namespace std;
using namespace DirectX;

FBXFileLoader::FBXFileLoader()
{
	m_sdkManager = FbxManager::Create();
	m_materialLastAddedID = 0;
	m_initialized = false;
}


FBXFileLoader::~FBXFileLoader()
{
	if (m_scene != NULL) m_scene->Destroy();
	m_sdkManager->Destroy();
}

void FBXFileLoader::Initialize(ObjectManager* mngrObject, ResourceManager* mngrResource, Utilit3D* utilit3D)
{
	assert(mngrObject);
	assert(mngrResource);
	assert(utilit3D);

	m_objectManager = mngrObject;	
	m_resourceManager = mngrResource;
	m_utilit3D = utilit3D;
	m_initialized = true;
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

	int child_count = m_scene->GetRootNode()->GetChildCount();
	for (int i = 0; i < child_count; i++)
		process_node(m_scene->GetRootNode()->GetChild(i));
		//build_Animation();
	build_GeoMeshes();

	m_resourceManager->buildTexurePathList();
	m_resourceManager->buildTexureFullNameList();
	m_resourceManager->loadTexture();
		//build_Skeleton();
	process_NodeInstances();
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
		std::vector<VertexExtGPU> meshVertices;
		std::vector<uint32_t> meshIndices;

		VertexExtGPU vertex = {};
		auto lgeoMesh = std::make_unique<Mesh>();

		fbx_Mesh* lMesh = mesh_it_begin->second.get();
		SubMesh submesh = {};

		lgeoMesh->Name = lMesh->Name;

		meshVertices.resize(lMesh->Indices.size());
		meshIndices.resize(lMesh->Indices.size());

		for (int vi = 0; vi < lMesh->Indices.size(); vi++)
		{
			vertex = {};
			vertex.Pos = lMesh->Vertices[lMesh->Indices[vi]];
			if (lMesh->Normals.size())
			{
				XMVECTOR n = XMLoadFloat3(&lMesh->Normals[vi]);
				n = XMVector3Normalize(n);
				XMStoreFloat3(&vertex.Normal, n);
			}
			if (lMesh->UVs.size())	vertex.UVText = lMesh->UVs[vi];
			vertex.ShapeID = 0;

			// write vertex/bone weight information			

			for (int wi = 0; wi < lMesh->VertexWeightByBoneName[lMesh->Indices[vi]].size(); wi++)
			{
				string lBoneName = lMesh->VertexWeightByBoneName[lMesh->Indices[vi]][wi].first;
				float lBoneWeight = lMesh->VertexWeightByBoneName[lMesh->Indices[vi]][wi].second;
				int lBoneID = m_BonesIDByName[lBoneName].first;
				vertex.BoneIndices[wi] = lBoneID;
				vertex.BoneWeight[wi] = lBoneWeight;
			}

			meshVertices[vi] = vertex;
			meshIndices[vi] = vi;
		}

		submesh.IndexCount = meshVertices.size();
		lgeoMesh->DrawArgs[lMesh->Name] = submesh;

		//Create RenderItem for this mesh		
		m_RenderItems[lMesh->Name] = std::make_unique<RenderItem>();
		RenderItem& lNewRI = *m_RenderItems[lMesh->Name].get();
		BoundingBox::CreateFromPoints(lNewRI.AABB, lMesh->Vertices.size(), &lMesh->Vertices[0], sizeof(lMesh->Vertices[0]));
		lNewRI.Name = lMesh->Name;
		lNewRI.Geometry = lgeoMesh.get();

		if (lMesh->VertexPerPolygon == 3 || lMesh->VertexPerPolygon == 4)
			lNewRI.Geometry->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		else
			assert(0);

		//upload geoMesh to GPU
		
		m_utilit3D->UploadMeshToDefaultBuffer<Mesh, VertexExtGPU, uint32_t>(lgeoMesh.get(), meshVertices, meshIndices);

		//move geoMeshUp		
	 	m_objectManager->addMesh(lgeoMesh->Name, lgeoMesh);
	}
}

void FBXFileLoader::build_Animation()
{
	// get animation steck count
	int lAnimationStackCount = m_scene->GetSrcObjectCount<FbxAnimStack>(); // (FBX_TYPE(FbxAnimStack)); i++);
	//for (int i = 0; i < lAnimationStackCount; i++)

	if (lAnimationStackCount == 0) return;
	FbxAnimStack* lpAnimStack = FbxCast<FbxAnimStack>(m_scene->GetSrcObject<FbxAnimStack>(2)); //run 3
	string lAnimStackName = lpAnimStack->GetName();
	int lAnimLayerCount = lpAnimStack->GetMemberCount<FbxAnimLayer>();
	int a = 1;

	FbxAnimLayer* lAnimLayer = lpAnimStack->GetMember<FbxAnimLayer>(0);

	m_scene->SetCurrentAnimationStack(lpAnimStack);

	//FbxNode* currentNode = m_scene->FindNodeByName("Bone");
	//assert(currentNode);

	//// get list of Animation Curve Nodes
	//FbxAnimCurve* lAnimCurve = currentNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);

	//int count = lAnimCurve->KeyGetCount();
	//FbxAnimCurveKey key =  lAnimCurve->KeyGet(1);

	//FbxTime time = key.GetTime();
	//float value =key.GetValue();
	//int a1 = 1;

}

void FBXFileLoader::build_Skeleton()
{
	//m_bones.resize(m_BonesIDByName.size());

	//auto begin_it = m_BonesIDByName.begin();
	//for (; begin_it != m_BonesIDByName.end(); begin_it++)
	//	m_bones[begin_it->second.first] = begin_it->second.second->Node;

	////m_fbxSkinnedData = std::make_unique<FbxSkinnedData>(&m_bones, m_bones.size(), m_scene);
	//m_fbxSkinnedData = std::make_unique<FbxSkinnedData>(m_rootBone, m_BonesIDByName.size(), m_scene);
	//addSkinnedData(m_fbxSkinnedData);
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

			for (int i = 0; i < (*mat_it).second.TexturesNameByType.size(); i++)
			{
				int liTextureType = -1;
				string type = (*mat_it).second.TexturesNameByType[i].first;
				string texture_name = (*mat_it).second.TexturesNameByType[i].second;
				
				if (type == "diffuse")
					liTextureType = 0;
				else if (type == "normal")
					liTextureType = 1;
				else if (type == "specular")
					liTextureType = 2;

				int textureID = m_resourceManager->getTexturePathIDByName(texture_name);
				if (liTextureType >= 0)
				{
					lMaterial->TexturesMask |= (1 << liTextureType);
					int index = (1 << liTextureType) - 1;
					lMaterial->DiffuseColorTextureIDs[index] = textureID;
				}
			}
			m_resourceManager->addMaterial(lMaterial);
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
		if (ri_it->second->Type == 0)
		{
			//define a type for RenderItem, let's use the first Instance for this and his material
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
			m_objectManager->addOpaqueObject(begin_it->second);

		else if (begin_it->second->Type == RenderItemType::RIT_Transparent)
			m_objectManager->addTransparentObject(begin_it->second);

		else if (begin_it->second->Type == RenderItemType::RIT_Animated)
			m_objectManager->addAnimatedObject(begin_it->second);
		else
			assert(0); 
		
	}
	//
}

int FBXFileLoader::getNextMaterialID()
{
	return  m_materialLastAddedID++;
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

	m = XMMatrixTranspose(m);
	//m = XMMatrixIdentity();
	XMStoreFloat4x4(&m4x4, m);
}

// ---------------- FBX functions -----------------------------
void FBXFileLoader::process_node(const FbxNode* pNode)
{
	string nodeName = pNode->GetName();
	const FbxNodeAttribute* pNodeAtrib = pNode->GetNodeAttribute();
	int atribCount = pNode->GetNodeAttributeCount();

	fbx_NodeInstance newNodeInstance = {};
	newNodeInstance.MeshName = pNodeAtrib->GetName();

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
					FbxDouble* lDifuseData = lphongMaterial->Diffuse.Get().Buffer();
					float f1 = lphongMaterial->Diffuse.Get().mData[0];
					float f2 = lphongMaterial->Diffuse.Get().mData[1];
					float f3 = lphongMaterial->Diffuse.Get().mData[2];
					float f4 = lphongMaterial->AmbientFactor;

					lMaterial.DiffuseAlbedo = XMFLOAT4(f1, f2, f3, f4);
					int srcObjectcount = lphongMaterial->Diffuse.GetSrcObjectCount<FbxTexture>();

					if (srcObjectcount)
					{
						// We do not support layered texture now, so just take the first texture
						FbxTexture* lTexture = lphongMaterial->Diffuse.GetSrcObject<FbxTexture>(0);
						assert(lTexture);

						string lTextureName = lTexture->GetName();
						FbxFileTexture* lFileTexture = FbxCast<FbxFileTexture>(lTexture);
						assert(lFileTexture);
						string lTexturePath = lFileTexture->GetFileName();
						
						m_resourceManager->addTexturePathByName(lTextureName, lTexturePath);
						
						auto ltextureTypeName = std::make_pair("diffuse", lTextureName);
						lMaterial.TexturesNameByType.push_back(ltextureTypeName);
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
			if (m_rootBone == NULL)
			{

				m_rootBone = new fbx_TreeBoneNode();
				FbxNode* parentNode = const_cast<FbxNode*>(pNode->GetParent());
				std::string name = parentNode->GetName();
				m_rootBone->Name = name;
				m_rootBone->Node = parentNode;
				m_BonesIDByName[name].first = m_BoneGlobalID++;
				m_BonesIDByName[name].second = m_rootBone;

				process_Skeleton(pNode, m_rootBone);
			}
			else
				assert(0);// we should not come here one more time
		}
		break;
		case fbxsdk::FbxNodeAttribute::eMesh:
		{
			process_mesh(pNodeAtrib);

			FbxDouble3 transaltion = pNode->LclTranslation;
			FbxDouble3 rotation = pNode->LclRotation;
			FbxDouble3 scaling = pNode->LclScaling;

			FbxNode* lNode2 = const_cast<FbxNode*>(pNode);
			string name = lNode2->GetName();

			FbxAMatrix lGlobalTransform = lNode2->EvaluateGlobalTransform();
			FbxAMatrix lLocalTransform = lNode2->EvaluateGlobalTransform();

			convertFbxMatrixToFloat4x4(lGlobalTransform, newNodeInstance.GlobalTransformation);
			convertFbxMatrixToFloat4x4(lLocalTransform, newNodeInstance.LocalTransformation);
			newNodeInstance.Nodetype = NT_Mesh;
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
	const FbxMesh* lpMesh = static_cast<const FbxMesh*>(pNodeAtribute);
	string name = lpMesh->GetName();

	if (m_meshesByName.find(name) != m_meshesByName.end()) return;

	FbxVector4* vertexData = lpMesh->GetControlPoints();
	int* indicesData = lpMesh->GetPolygonVertices(); //get a point to Indices data
	int polygon_vert_count = lpMesh->GetPolygonVertexCount(); // get count of indices
	int vertex_count = lpMesh->GetControlPointsCount();//get count unique vertices
	int polygon_count = lpMesh->GetPolygonCount();// how many polygons we have
	int polygon_size = lpMesh->GetPolygonSize(0); // how many vertices uses a one polygon

	assert(polygon_vert_count == (polygon_size* polygon_count)); // we think that all polygons should have the same size
	assert(polygon_size <= 4);

	auto lmesh = std::make_unique<fbx_Mesh>();
	lmesh->Name = name;
	lmesh->VertexPerPolygon = polygon_size;

	for (int i = 0; i < vertex_count; i++, vertexData++)
		lmesh->Vertices.push_back(XMFLOAT3(vertexData->mData[0], vertexData->mData[1], vertexData->mData[2]));

	for (int i = 0; i < polygon_vert_count; i++)
	{
		if (polygon_size == 4)
		{
			int i0 = *(indicesData++);
			int i1 = *(indicesData++);
			int i2 = *(indicesData++);
			int i3 = *(indicesData++);

			lmesh->Indices.push_back(i0);
			lmesh->Indices.push_back(i1);
			lmesh->Indices.push_back(i2);

			lmesh->Indices.push_back(i0);
			lmesh->Indices.push_back(i2);
			lmesh->Indices.push_back(i3);
			i += 3;
		}
		else
			lmesh->Indices.push_back(*(indicesData++));
	}

	// get Material ID for mesh/polygons
	{
		int count = lpMesh->GetElementMaterialCount(); //just to know. now we will use the 0 material;
		const FbxLayerElementMaterial* materialLayer = lpMesh->GetElementMaterial();
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

	//Normals
	{
		int count = lpMesh->GetElementNormalCount();
		const FbxLayerElementNormal* normalLayer = lpMesh->GetElementNormal();
		FbxGeometryElement::EMappingMode mappingMode = normalLayer->GetMappingMode();
		FbxGeometryElement::EReferenceMode referenceMode = normalLayer->GetReferenceMode();

		if (mappingMode == FbxGeometryElement::EMappingMode::eByPolygonVertex &&
			referenceMode == FbxGeometryElement::EReferenceMode::eDirect)
		{
			int normalCount = normalLayer->GetDirectArray().GetCount();

			//// We should sum all normals for the same vertex and normalize it later
			vector<XMFLOAT3> averageNormal(lmesh->Vertices.size());
			indicesData = lpMesh->GetPolygonVertices(); //get a point to Indices data
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

			indicesData = lpMesh->GetPolygonVertices(); //get a point to Indices data
			for (int i = 0; i < normalCount; i++)
			{
				if (polygon_size == 4)
				{
					XMFLOAT3 normal0 = averageNormal[*(indicesData++)];
					XMFLOAT3 normal1 = averageNormal[*(indicesData++)];
					XMFLOAT3 normal2 = averageNormal[*(indicesData++)];
					XMFLOAT3 normal3 = averageNormal[*(indicesData++)];

					lmesh->Normals.push_back(normal0);
					lmesh->Normals.push_back(normal1);
					lmesh->Normals.push_back(normal2);

					lmesh->Normals.push_back(normal0);
					lmesh->Normals.push_back(normal2);
					lmesh->Normals.push_back(normal3);
					i += 3;
				}
				else
				{
					//FbxVector4 normal = normalLayer->GetDirectArray().GetAt(i);
					//lmesh->Normals.push_back(XMFLOAT3(normal.mData[0], normal.mData[1], normal.mData[2]));

					XMFLOAT3 normal = averageNormal[*(indicesData++)];
					lmesh->Normals.push_back(normal);
				}
			}
		}
		else
		{
			bool NormalNotByVertexPolygon = false;
			assert(NormalNotByVertexPolygon);
		}
	}

	//UV
	{
		int count = lpMesh->GetElementUVCount();
		const FbxLayerElementUV* UVLayer = lpMesh->GetElementUV();
		if (UVLayer)
		{
			FbxGeometryElement::EMappingMode mappingMode = UVLayer->GetMappingMode();
			FbxGeometryElement::EReferenceMode referenceMode = UVLayer->GetReferenceMode();

			if (mappingMode == FbxGeometryElement::EMappingMode::eByPolygonVertex &&
				referenceMode == FbxGeometryElement::EReferenceMode::eIndexToDirect)
			{
				int uvCount = UVLayer->GetIndexArray().GetCount();

				for (int i = 0; i < uvCount; i++)
				{
					if (polygon_size == 4)
					{
						int uvID0 = UVLayer->GetIndexArray().GetAt(i++);
						int uvID1 = UVLayer->GetIndexArray().GetAt(i++);
						int uvID2 = UVLayer->GetIndexArray().GetAt(i++);
						int uvID3 = UVLayer->GetIndexArray().GetAt(i);

						FbxVector2 uv0 = UVLayer->GetDirectArray().GetAt(uvID0);
						FbxVector2 uv1 = UVLayer->GetDirectArray().GetAt(uvID1);
						FbxVector2 uv2 = UVLayer->GetDirectArray().GetAt(uvID2);
						FbxVector2 uv3 = UVLayer->GetDirectArray().GetAt(uvID3);

						lmesh->UVs.push_back(XMFLOAT2(uv0.mData[0], 1.0f - uv0.mData[1]));
						lmesh->UVs.push_back(XMFLOAT2(uv1.mData[0], 1.0f - uv1.mData[1]));
						lmesh->UVs.push_back(XMFLOAT2(uv2.mData[0], 1.0f - uv2.mData[1]));

						lmesh->UVs.push_back(XMFLOAT2(uv0.mData[0], 1.0f - uv0.mData[1]));
						lmesh->UVs.push_back(XMFLOAT2(uv2.mData[0], 1.0f - uv2.mData[1]));
						lmesh->UVs.push_back(XMFLOAT2(uv3.mData[0], 1.0f - uv3.mData[1]));
					}
					else
					{
						int uvID = UVLayer->GetIndexArray().GetAt(i);
						FbxVector2 uv = UVLayer->GetDirectArray().GetAt(uvID);
						lmesh->UVs.push_back(XMFLOAT2(uv.mData[0], uv.mData[1]));
					}
				}
			}
			else
			{
				bool Mesh_UV_MappingRef = false;
				assert(Mesh_UV_MappingRef);
			}
		}
	}

	// get Skin Data
	// If Bone X uses vertex Y with weight W, we will map it with VertexWeightByBoneID[Y]=pair{X, W}
	lmesh->VertexWeightByBoneName.resize(lmesh->Vertices.size());
	{
		int lCommonIndicesCount = 0;
		int lskinCount = lpMesh->GetDeformerCount(FbxDeformer::eSkin);

		for (int s = 0; s < lskinCount; s++)
		{
			FbxSkin* lSkin = (FbxSkin*)lpMesh->GetDeformer(s, FbxDeformer::eSkin);
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