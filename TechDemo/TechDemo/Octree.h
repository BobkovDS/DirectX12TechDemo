#pragma once
#include "ApplDataStructures.h"

class Octree
{
	DirectX::BoundingBox m_Cub_bb; // Bounding box for this Octree Cub node
	std::vector<Octree*> m_childs; // we will check only these sub-kub which we have
	std::list<BoundingBoxEXT*> m_listOfContainedBB;	

	Octree* getNextChild(int childID);
	bool checkContainment(BoundingBoxEXT* bb);
	bool checkChildContainment(BoundingBoxEXT* bb);
	void deleteChilds();
	void addBB(BoundingBoxEXT* bb);
public:
	Octree(DirectX::BoundingBox m_Cub_bb);
	~Octree();
	
	void build();
	void update(DirectX::BoundingFrustum& frustom);
	bool isContainListBBEmpty() { return m_listOfContainedBB.size() == 0; }
	void addBBList(std::vector<BoundingBoxEXT*>& iBBlist);
	
	void getBBListForIntersection(std::vector<DirectX::BoundingBox*>& bbList,
		DirectX::FXMVECTOR& rayOrigin, DirectX::FXMVECTOR& rayDir, float dist);

	void getInstancesList();
};

