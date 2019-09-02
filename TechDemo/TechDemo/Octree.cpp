#include "Octree.h"
#include "Scene.h"

using namespace std;
using namespace BoundingMath;

DirectX::XMVECTOR BoundingMath::BoundingFrustum::sNearPlane;
DirectX::XMVECTOR BoundingMath::BoundingFrustum::sFarPlane;
DirectX::XMVECTOR BoundingMath::BoundingFrustum::sLeftPlane;
DirectX::XMVECTOR BoundingMath::BoundingFrustum::sRightPlane;
DirectX::XMVECTOR BoundingMath::BoundingFrustum::sBottomPlane;
DirectX::XMVECTOR BoundingMath::BoundingFrustum::sTopPlane;

OctreeSelector Octree::selector;

Octree::Octree(BoundingMath::BoundingBox iCub_bb): m_Cub_bb(iCub_bb)
{
}

Octree::~Octree()
{
}

Octree* Octree::getNextChild(int iCharID)
{
	//int child_id = m_childs.size();
	int child_id = iCharID;
	
	assert(child_id < 8);

	DirectX::XMFLOAT3 corners[8];
	m_Cub_bb.GetCorners(corners);

	DirectX::XMVECTOR childP1 = DirectX::XMLoadFloat3(&corners[child_id]);
	DirectX::XMVECTOR childP2 = DirectX::XMLoadFloat4(&m_Cub_bb.Center);
	
	DirectX::BoundingBox childBB;
	DirectX::BoundingBox::CreateFromPoints(childBB, childP1, childP2);
		//DirectX::BoundingBox::CreateFromPoints(childBB, childP1, childP2);

	BoundingBox lChildBB(childBB);
	return new Octree(lChildBB);
}

bool Octree::checkContainment(BoundingBoxEXT* bb)
{	
	ContainmentType lResult = m_Cub_bb.contains(*bb);
	return (lResult == CONTAINS); 
}

bool Octree::checkChildContainment(BoundingBoxEXT* bb)
{
	int childID = 0;
	while (childID<8)
	{
		if (m_childs.size() == childID) m_childs.push_back(getNextChild(childID)); //create new child

		if (m_childs[childID]->checkContainment(bb))
		{
			m_childs[childID]->addBB(bb);
			return true;
		}
		childID++;
	}
	return false;
}

void Octree::addBB(BoundingBoxEXT* bb)
{
	auto it_end = m_listOfContainedBB.end();
	m_listOfContainedBB.insert(it_end, bb);
}

void Octree::addBBList(std::vector<BoundingBoxEXT*>& iBBlist)
{	
	copy(iBBlist.begin(), iBBlist.end(), back_inserter(m_listOfContainedBB));
}

void Octree::build()
{
	if (m_listOfContainedBB.size() <= 100)
	{
		m_vectorContainedBB.resize(m_listOfContainedBB.size());
		copy(m_listOfContainedBB.begin(), m_listOfContainedBB.end(), m_vectorContainedBB.begin());
		m_listOfContainedBB.clear(); // we do not need list any more;
		return;
	}

	bool anyBBWasAddedToAnyChild = false;
	Octree* child;
	auto it = m_listOfContainedBB.begin();
	auto it_end = m_listOfContainedBB.end();

	//for each (BoundingBox* var in m_listOfContainedBB)

	while ( it!= it_end)
	{
		if (checkChildContainment(*it))
		{		
			auto removIT = it++;
			
			m_listOfContainedBB.remove(*removIT);
			anyBBWasAddedToAnyChild = true;
		}
		else it++;
	}

	m_vectorContainedBB.resize(m_listOfContainedBB.size());
	copy(m_listOfContainedBB.begin(), m_listOfContainedBB.end(), m_vectorContainedBB.begin());
	m_listOfContainedBB.clear();

	if (!anyBBWasAddedToAnyChild)
		deleteChilds();
	else
	{
		// Delete empty childs
		int lChildID = 0;
		for (int i = 0; i < m_childs.size(); i++)
		{
			if (!m_childs[i]->isContainListBBEmpty())			
				m_childs[lChildID++] = m_childs[i];			
		}
		m_childs.resize(lChildID);

		// Check Childs BB list
		for (int i = 0; i < m_childs.size(); i++)
			m_childs[i]->build();
	}
}

void Octree::deleteChilds() 
{
	for (int i = 0; i < m_childs.size(); i++)	
		delete m_childs[i];
	m_childs.clear();
}

void Octree::update(BoundingFrustum& frustom)
{	
	// 1) If BB intersects Shadow Frustom
	
	if (frustom.intersects(m_Cub_bb))
	{		
		// Save effective Radii for childs calls (for sub-octree nodes we do not need to calculate effective Radii again. just use half for this)
		float r1 = frustom.EffR1 / 2.0f;
		float r2 = frustom.EffR2 / 2.0f;
		float r3 = frustom.EffR3 / 2.0f;
		float r4 = frustom.EffR4 / 2.0f;
		float r5 = frustom.EffR5 / 2.0f;

		// 2.1) Check all Intaneces for this BB		
		for (int i=0; i< m_vectorContainedBB.size(); i++)		
		{
			BoundingBoxEXT& lInstBB = *m_vectorContainedBB[i];			

			bool f1 = frustom.intersects(lInstBB);
			
			if (f1)			
			{
				using namespace DirectX;
				DirectX::XMVECTOR lBBCenter = DirectX::XMLoadFloat4(&lInstBB.Center);				
				DirectX::XMVECTOR lR = lBBCenter - selector.SelectorPostition;				
				DirectX::XMVECTOR lRLenght = DirectX::XMVector3Length(lR);

				float lRDistance = DirectX::XMVectorGetX(lRLenght);

				std::pair<UINT, UINT> lID_LOD;

				if (lInstBB.pRenderItem->Geometry != NULL) // If we do not use LOD for this RI, so lets draw all instances in LOD0
					lID_LOD.second = 0;
				else // ..but if we have LOD for this RI
				{
					if (lRDistance <= selector.LOD0_distance)
						lID_LOD.second = 0;
					else if (lRDistance > selector.LOD1_distance)
						lID_LOD.second = 2;
					else
						lID_LOD.second = 1;
				}

				lID_LOD.first = lInstBB.Inst_ID;

				lInstBB.pRenderItem->InstancesID_LOD[lID_LOD.second][lInstBB.pRenderItem->InstancesID_LOD_size[lID_LOD.second]++]
					= lID_LOD.first; 
			}
		}

		// 2.2) Check Childs for Shadow Frustom only
		for (int i = 0; i < m_childs.size(); i++)
			m_childs[i]->update(frustom, r1, r2, r3, r4, r5);
	}	
}

void Octree::update(BoundingFrustum& frustom, float r1, float r2, float r3, float r4, float r5)
{
	// 1) If BB intersects Shadow Frustom

	//if (frustom.intersects(m_Cub_bb.Center, r1, r2, r3, r4, r5))
	BoundingMath::ContainmentType lCheckResult = frustom.contains(m_Cub_bb.Center, r1, r2, r3, r4, r5);

	if (lCheckResult != BoundingMath::ContainmentType::DISJOINT)
	{
		// 2.1) Check all Intaneces for this BB

		for (int i = 0; i < m_vectorContainedBB.size(); i++)
		{
			BoundingBoxEXT& lInstBB = *m_vectorContainedBB[i];

			bool f1 = true;
			if (lCheckResult == BoundingMath::ContainmentType::INTERSECTS)
				f1 = frustom.intersects(lInstBB);

			if (f1)
			{
				using namespace DirectX;
				DirectX::XMVECTOR lBBCenter = DirectX::XMLoadFloat4(&lInstBB.Center);
				DirectX::XMVECTOR lR = lBBCenter - selector.SelectorPostition;
				DirectX::XMVECTOR lRLenght = DirectX::XMVector3Length(lR);

				float lRDistance = DirectX::XMVectorGetX(lRLenght);

				std::pair<UINT, UINT> lID_LOD;

				if (lInstBB.pRenderItem->Geometry != NULL) // If we do not use LOD for this RI, so lets draw all instances in LOD0
					lID_LOD.second = 0;
				else // ..but if we have LOD for this RI
				{
					if (lRDistance <= selector.LOD0_distance)
						lID_LOD.second = 0;
					else if (lRDistance > selector.LOD1_distance)
						lID_LOD.second = 2;
					else
						lID_LOD.second = 1;
				}

				lID_LOD.first = lInstBB.Inst_ID;

				lInstBB.pRenderItem->InstancesID_LOD[lID_LOD.second][lInstBB.pRenderItem->InstancesID_LOD_size[lID_LOD.second]++]
					= lID_LOD.first;
			}
		}

		r1 *= 0.5f;
		r2 *= 0.5f;
		r3 *= 0.5f;
		r4 *= 0.5f;
		r5 *= 0.5f;

		// 2.2) Check Childs for Shadow Frustom only
		for (int i = 0; i < m_childs.size(); i++)
			m_childs[i]->update(frustom, r1, r2, r3, r4, r5 );
	}
}


//void Octree::update()
//{
//	for (int i = 0; i < m_vectorContainedBB.size(); i++)
//	{
//		BoundingBoxEXT* lInstBB = m_vectorContainedBB[i];
//		XMVECTOR lBBCenter = XMLoadFloat3(&lInstBB->Center);
//		XMVECTOR lR = lBBCenter - selector.SelectorPostition;
//		XMVECTOR lRLenght = XMVector3Length(lR);
//		//XMVECTOR lR_check = XMVector3Normalize(lR); // ONLY TO CHECK NORMALIZATION
//		lR = lR / lRLenght;
//
//		XMVECTOR lDot = XMVector3Dot(selector.SelectorDirection, lR);
//
//		float lAngle = XMVectorGetX(lDot);
//
//		if (lAngle >= selector.ConeCosA) // Now we use Cone Variant		
//		{
//			float lRDistance = XMVectorGetX(lRLenght);
//
//			std::pair<UINT, UINT> lID_LOD;
//
//			if (lInstBB->pRenderItem->Geometry != NULL) // If we do not use LOD for this RI, so lets draw all instances in LOD0
//				lID_LOD.second = 0;
//			else // ..but if we have LOD for this RI
//			{
//				if (lRDistance <= selector.LOD0_distance)
//					lID_LOD.second = 0;
//				else if (lRDistance > selector.LOD1_distance)
//					lID_LOD.second = 2;
//				else
//					lID_LOD.second = 1;
//			}
//
//			lID_LOD.first = m_vectorContainedBB[i]->Inst_ID;
//			lInstBB->pRenderItem->InstancesID_LOD[lID_LOD.second][lInstBB->pRenderItem->InstancesID_LOD_size[lID_LOD.second]++]
//				= lID_LOD.first; // add InstanceID and LOD
//		}
//
//	}
//
//	for (int i = 0; i < m_childs.size(); i++)
//	{
//		BoundingBox& lChildNodeBB = m_childs[i]->m_Cub_bb;
//		XMVECTOR lBBCenter = XMLoadFloat3(&lChildNodeBB.Center);
//		XMVECTOR lR = lBBCenter - selector.SelectorPostition;
//		XMVECTOR lRLenght = XMVector3Length(lR);		
//		lR = lR / lRLenght;
//
//		XMVECTOR lDot = XMVector3Dot(selector.SelectorDirection, lR);
//
//		float lAngle = XMVectorGetX(lDot);
//
//		if (lAngle >= 0) // Now we use Cone Variant		
//			m_childs[i]->update();
//	}
//}
//
//void Octree::getBBListForIntersection(std::vector<BoundingBox*>& bbList,
//	DirectX::FXMVECTOR& rayOrigin, DirectX::FXMVECTOR& rayDir, float dist)
//{
//	// 1) Give all BB for current sub-Cub
//	copy(m_listOfContainedBB.begin(), m_listOfContainedBB.end(), back_inserter(bbList));
//
//	// 2) Lets childs do the same if their BB intersect with ray
//	for (int i = 0; i < m_childs.size(); i++)
//	{
//		if (m_childs[i]->m_Cub_bb.Intersects(rayOrigin, rayDir, dist))
//			m_childs[i]->getBBListForIntersection(bbList, rayOrigin, rayDir, dist);
//	}
//}
//
//void Octree::getInstancesList()
//{
//	auto it_begin = m_listOfContainedBB.begin();
//	auto it_end = m_listOfContainedBB.end();
//
//	for (; it_begin != it_end; it_begin++)
//		(*it_begin)->pRenderItem->InstancesID.push_back((*it_begin)->Inst_ID);
//	
//	for (int i = 0; i < m_childs.size(); i++)
//		m_childs[i]->getInstancesList();
//}

std::vector<BoundingBoxEXT*>& Octree::getContainedBBVector()
{
	return m_vectorContainedBB;
}

int Octree::getBBCount()
{
	int lBBcount = m_vectorContainedBB.size();
	//int lBBcount = m_listOfContainedBB.size();

	for (int i = 0; i < m_childs.size(); i++)
		lBBcount += m_childs[i]->getBBCount();

	return lBBcount;
}