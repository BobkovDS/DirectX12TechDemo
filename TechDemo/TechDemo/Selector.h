#pragma once
#include "ApplDataStructures.h"

class Selector
{
	std::vector<BoundingBoxEXT*>* m_listOfBB;

	float m_verticalAngle;
	float m_horizontalAngle;
	float m_coneCosA;
	
	// LOD distances
	float m_LOD0_distance;
	float m_LOD1_distance;	

	DirectX::XMFLOAT3 m_selectorPostition;
	DirectX::XMFLOAT3 m_selectorDirection;
	
public:
	Selector();
	~Selector();

	void update();
	void initialize(float LOD0, float LOD1);
	void setBBList(std::vector<BoundingBoxEXT*>& iBBlist);
	void set_coinAngle(float angle);
	void set_selector_position(DirectX::XMFLOAT3 pos);
	void set_selector_direction(DirectX::XMFLOAT3 dir);
	void set_selector_direction(DirectX::XMVECTOR dir);
};

