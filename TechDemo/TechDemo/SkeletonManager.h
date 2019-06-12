#pragma once
#include "SkinnedData.h"
#include <map>

class SkeletonManager
{
	std::map<std::string, SkinnedData> m_skeletons;

public:	

	SkeletonManager();
	~SkeletonManager();

	//SkinnedData* addSkeleton(std::string& skeletonName); /*TO_DO: delete*/
	SkinnedData& getSkeleton(std::string& skeletonName);
	SkinnedData& getSkeleton(UINT i);
	int getSkeletonCount();
};

