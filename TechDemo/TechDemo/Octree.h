/*
	***************************************************************************************************
	Description:
		Octree realization.
		 - It uses custum Bounding Objects to have Intersection checking more effective.
		 - For internal objects for specific BoundingBox it calculates LODs level for these objects. 
	***************************************************************************************************
*/

#pragma once
#include "ApplDataStructures.h"
#include "BoundingMath.h"

struct OctreeSelector {	
		// LOD distances
		float LOD0_distance;
		float LOD1_distance;

		DirectX::XMVECTOR SelectorPostition; // for operation only
		DirectX::XMVECTOR SelectorDirection; // for operation only
};

class Octree
{
	BoundingMath::BoundingBox m_Cub_bb; // Bounding box for this Octree Cub node
	std::vector<Octree*> m_childs; // we will check only these sub-kub which we have
	std::list<BoundingMath::BoundingBoxEXT*> m_listOfContainedBB;	// for storing all BB at first
	std::vector<BoundingMath::BoundingBoxEXT*> m_vectorContainedBB; // to store BBs, which were not inluced to Childs, m_listOfContainedBB are not used after this anymore

	Octree* getNextChild(int childID);
	bool checkContainment(BoundingMath::BoundingBoxEXT* bb);
	bool checkChildContainment(BoundingMath::BoundingBoxEXT* bb);
	void deleteChilds();
	void addBB(BoundingMath::BoundingBoxEXT* bb);
public:
	Octree(BoundingMath::BoundingBox m_Cub_bb);
	~Octree();
	
	static OctreeSelector selector;
	void build();
	void update(BoundingMath::BoundingFrustum& frustom);
	void update(BoundingMath::BoundingFrustum& frustom, float r1, float r2, float r3, float r4, float r5 );	
	bool isContainListBBEmpty() { return m_listOfContainedBB.size() == 0; }
	void addBBList(std::vector<BoundingMath::BoundingBoxEXT*>& iBBlist);	
};

