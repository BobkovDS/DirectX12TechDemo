#include "Selector.h"
using namespace DirectX;
//
//Selector::Selector()
//{
//}
//
//Selector::~Selector()
//{
//}
//
//void Selector::initialize(float LOD0, float LOD1)
//{
//	m_LOD0_distance = LOD0;
//	m_LOD1_distance = LOD1;	
//}
//
//void Selector::update()
//{
//	XMVECTOR lSelectorPosition = XMLoadFloat3(&m_selectorPostition);
//	XMVECTOR lSelectorDirection = XMLoadFloat3(&m_selectorDirection);
//	XMMATRIX lviewMatrix = XMLoadFloat4x4(&m_viewM);
//
//	std::vector<BoundingBoxEXT*>& ListOfBB = *m_listOfBB;	
//
//	assert(m_listOfBB != NULL);
//	for (int i=0; i<m_listOfBB->size(); i++)
//	{			
//		BoundingBoxEXT* lInstBB = ListOfBB[i];
//		BoundingBox lInstBBViewSpace(*lInstBB);
//		lInstBB->Transform(lInstBBViewSpace, lviewMatrix);
//
//		BoundingSphere lBSphere;
//		BoundingSphere::CreateFromBoundingBox(lBSphere, lInstBBViewSpace);
//
//		BoundingMath::BoundingSphere lshp;
//		lshp.build(lBSphere.Radius, lBSphere.Center);
//		
//		BoundingMath::BoundingBox lbox;
//		lbox.build(lInstBBViewSpace.Center, lInstBBViewSpace.Extents);
//
//		bool lIntersect = m_cameraFrustum->intersects(lshp);
//		bool lIntersect2 = m_cameraFrustumFromPorojM.Intersects(lBSphere);
//
//		bool lIntersectBox = m_cameraFrustum->intersects(lbox);
//		bool lIntersectBox2 = m_cameraFrustumFromPorojM.Intersects(lInstBBViewSpace);
//
//		XMVECTOR lBBCenter = XMLoadFloat3(&lInstBB->Center);
//		XMVECTOR lR = lBBCenter - lSelectorPosition;
//		XMVECTOR lRLenght = XMVector3Length(lR);
//		//XMVECTOR lR_check = XMVector3Normalize(lR); // ONLY TO CHECK NORMALIZATION
//		lR = lR / lRLenght;
//		
//		XMVECTOR lDot = XMVector3Dot(lSelectorDirection, lR);
//
//		float lAngle = XMVectorGetX(lDot);
//		
//		//if (lAngle >= m_coneCosA) // Now we use Cone Variant		
//		if (lIntersectBox2) // Now we use Cone Variant		
//		{
//			float lRDistance = XMVectorGetX(lRLenght);
//
//			std::pair<UINT, UINT> lID_LOD;
//
//			if (lInstBB->pRenderItem->Geometry != NULL) // If we do not use LOD for this RI, so lets draw all instances in LOD0
//				lID_LOD.second = 0;					   
//			else // ..but if we have LOD for this RI
//			{
//				if (lRDistance <= m_LOD0_distance)
//					lID_LOD.second = 0;
//				else if (lRDistance > m_LOD1_distance)
//					lID_LOD.second = 2;
//				else
//					lID_LOD.second = 1;
//			}
//
//			lID_LOD.first = ListOfBB[i]->Inst_ID;
//			
//			lInstBB->pRenderItem->InstancesID_LOD[lID_LOD.second][lInstBB->pRenderItem->InstancesID_LOD_size[lID_LOD.second]++]
//				=lID_LOD.first; // RenderItem.InstancesID_LOD[LOD_ID].add(Instance_ID)
//		}			
//	}
//}
//
//void Selector::setBBList(std::vector<BoundingBoxEXT*>& iBBlist)
//{
//	m_listOfBB = &iBBlist;
//}
//
//void Selector::set_coinAngle(float angle)
//{
//	// angle in Radians
//
//	m_coneCosA = cos(angle * XM_PI/180);
//}
//
//void Selector::set_selector_position(XMFLOAT3 pos)
//{
//	m_selectorPostition = pos;
//}
//
//void Selector::set_selector_direction(XMFLOAT3 dir)
//{
//	m_selectorDirection= dir;
//}
//
//void Selector::set_selector_direction(XMVECTOR dir)
//{
//	XMStoreFloat3(&m_selectorDirection, XMVector3Normalize(dir)); // TO_DO: should be Normalization here?
//}
//
//void Selector::set_viewMatrix(DirectX::XMFLOAT4X4 viewMatrix)
//{
//	m_viewM = viewMatrix;
//}
//
//void Selector::set_cameraFrustum(BoundingMath::BoundingFrustum* cameraFrustum)
//{
//	m_cameraFrustum = cameraFrustum;
//}
//
//void Selector::set_cameraFrustumFromProj(DirectX::BoundingFrustum cameraFrustumFromPorojM)
//{
//	m_cameraFrustumFromPorojM = cameraFrustumFromPorojM;
//}