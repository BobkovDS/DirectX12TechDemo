#pragma once
#include "ApplDataStructures.h"

struct OctreeSelector {	
		//float VerticalAngle;
		//float HorizontalAngle;
		float ConeCosA;

		// LOD distances
		float LOD0_distance;
		float LOD1_distance;

		DirectX::XMVECTOR SelectorPostition; // for operation only
		DirectX::XMVECTOR SelectorDirection; // for operation only
};

class Octree
{
	DirectX::BoundingBox m_Cub_bb; // Bounding box for this Octree Cub node
	std::vector<Octree*> m_childs; // we will check only these sub-kub which we have
	std::list<BoundingBoxEXT*> m_listOfContainedBB;	// for storing all BB at first
	std::vector<BoundingBoxEXT*> m_vectorContainedBB; // to store BBs, which were not inluced to Childs, m_listOfContainedBB are not used after this anymore

	Octree* getNextChild(int childID);
	bool checkContainment(BoundingBoxEXT* bb);
	bool checkChildContainment(BoundingBoxEXT* bb);
	void deleteChilds();
	void addBB(BoundingBoxEXT* bb);
public:
	Octree(DirectX::BoundingBox m_Cub_bb);
	~Octree();
	
	static OctreeSelector selector;
	void build();
	void update(DirectX::BoundingFrustum& frustom);	
	void update();	
	bool isContainListBBEmpty() { return m_listOfContainedBB.size() == 0; }
	void addBBList(std::vector<BoundingBoxEXT*>& iBBlist);
	
	std::vector<BoundingBoxEXT*>& getContainedBBVector();
	void getBBListForIntersection(std::vector<DirectX::BoundingBox*>& bbList,
		DirectX::FXMVECTOR& rayOrigin, DirectX::FXMVECTOR& rayDir, float dist);

	void getInstancesList();
	int getBBCount();
};

