#pragma once

#include "DirectXMath.h"
#include "ApplDataStructures.h"

namespace BoundingMath
{

	enum ContainmentType
	{
		DISJOINT = 0,
		INTERSECTS = 1,
		CONTAINS = 2
	};

/*******************************************************************
 - 
 - Declaration
 -
*******************************************************************/

	struct BoundingFrustum;
	struct BoundingSphere;
	struct BoundingBox;
//-----------------------------
// BoundingFrustom
//-----------------------------
	struct BoundingFrustum {
		float FocalLength;
		float AspectRatio;
		float HorizontalFieldOfView;
		float VerticalFieldOfView;
		float Near;
		float Far;

		//Planes
		DirectX::XMFLOAT4 NearPlane;
		DirectX::XMFLOAT4 FarPlane;
		DirectX::XMFLOAT4 LeftPlane;
		DirectX::XMFLOAT4 RightPlane;
		DirectX::XMFLOAT4 BottomPlane;
		DirectX::XMFLOAT4 TopPlane;

		static DirectX::XMVECTOR sNearPlane;
		static DirectX::XMVECTOR sFarPlane;
		static DirectX::XMVECTOR sLeftPlane;
		static DirectX::XMVECTOR sRightPlane;
		static DirectX::XMVECTOR sBottomPlane;
		static DirectX::XMVECTOR sTopPlane;

		// keepers for effective radii for BoundingBox intersections check
		float EffR1;
		float EffR2;
		float EffR3;
		float EffR4;
		float EffR5;

		//Creators
		BoundingFrustum() {};		

		bool intersects(const BoundingSphere& bc);
		bool intersects(const BoundingBox& bb);
		bool intersects(DirectX::XMFLOAT4& center, float r1, float r2, float r3, float r4, float r5);
		ContainmentType contains(DirectX::XMFLOAT4& center, float r1, float r2, float r3, float r4, float r5);

		void build(float horFV, float aspectRatio, float n, float f);
		void buildPlanes();		
		void setPlanes(DirectX::XMVECTOR& np, DirectX::XMVECTOR& fp, DirectX::XMVECTOR& lp, DirectX::XMVECTOR& rp,
			DirectX::XMVECTOR& bp, DirectX::XMVECTOR& tp);
		void getPlanesFromMatrix(BoundingFrustum& fr, DirectX::XMMATRIX& m);
		void transform(BoundingFrustum& fr, DirectX::XMMATRIX& m);
		
	};

//-----------------------------
// BoundingCircle
//-----------------------------
	struct BoundingSphere{
		float R;

		DirectX::XMFLOAT4 Center;
		DirectX::XMFLOAT3 basis_R;
		DirectX::XMFLOAT3 basis_S;
		DirectX::XMFLOAT3 basis_T;

		BoundingSphere() {};
		void build(float r, DirectX::XMFLOAT3 center);
	};

//-----------------------------
// BoundingBox
//-----------------------------
	struct BoundingBox {
		static const size_t CORNER_COUNT = 8;

		DirectX::XMFLOAT4 Center;
		DirectX::XMFLOAT3 Extents;
		DirectX::XMFLOAT3 basis_R;
		DirectX::XMFLOAT3 basis_S;
		DirectX::XMFLOAT3 basis_T;
		BoundingBox() {};		
		BoundingBox(const BoundingBox& bb);
		BoundingBox(const DirectX::BoundingBox& bb);		
		void build(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extents);
		void build(DirectX::XMFLOAT4 center, DirectX::XMFLOAT3 extents);
		float getEffectiveRadius(const DirectX::XMVECTOR& L) const;
		static void CreateMerged(BoundingBox& out, const BoundingBox& in1, const BoundingBox& in2);
		void GetCorners(DirectX::XMFLOAT3* Corners) const;
		ContainmentType contains(BoundingBox& bb);
	};


	struct BoundingBoxEXT : BoundingMath::BoundingBox
	{
	public:
		BoundingBoxEXT(const DirectX::BoundingBox& bb) : BoundingBox(bb) { }; //explicit		
		RenderItem* pRenderItem; // Identify the RenderItem, for which Instance this BB is using	
		UINT Inst_ID;	// Indentify the Instance (inside specific RI, for which this BB is using
	};

/*******************************************************************
 - 
 - Implementation
 -
*******************************************************************/

#include "BoundingMath.inl"

} // namespace BoundingMath