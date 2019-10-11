/*
	***************************************************************************************************
	Description:
	   Manages Skeletons for differents objects.
	   Skeletons are divided to two groups:
		- Skinned Skeletons: It is actually Skinned-animated skeleton: it has and bones and skin. 'Wolf' object is example for this.
		- NodeAnimated skeleton: it has only one bone and no skin. 'Light' object is example for this.

	***************************************************************************************************
*/

#pragma once
#include "SkinnedData.h"
#include <map>

class SkeletonManager
{
	std::map<std::string, SkinnedData> m_skeletons_SkinnedAnimated; // Skeletons for Skinned-animated object (Wolf)
	std::map<std::string, SkinnedData> m_skeletons_NodeAnimated; // skeletons for animated Objects like Camera or Light
	bool m_animationTimeWasEvaluated;
public:	

	SkeletonManager();
	~SkeletonManager();

	//SkinnedData* addSkeleton(std::string& skeletonName); /*TO_DO: delete*/
	SkinnedData& getSkeletonSkinnedAnimated(std::string& skeletonName);
	SkinnedData& getSkeletonSkinnedAnimated(UINT i);
	SkinnedData& getSkeletonNodeAnimated(std::string& skeletonName);
	SkinnedData& getSkeletonNodeAnimated(UINT i);
	bool isExistSkeletonNodeAnimated(std::string& skeletonName);
	int getSkeletonSkinnedAnimatedCount();
	int getSkeletonNodeAnimatedCount();
	
	void evaluateAnimationsTime();
};

