#pragma once

XMGLOBALCONST DirectX::XMVECTORF32 g_BoxOffset[8] =
{
	{ { { -1.0f, -1.0f,  1.0f, 0.0f } } },
	{ { {  1.0f, -1.0f,  1.0f, 0.0f } } },
	{ { {  1.0f,  1.0f,  1.0f, 0.0f } } },
	{ { { -1.0f,  1.0f,  1.0f, 0.0f } } },
	{ { { -1.0f, -1.0f, -1.0f, 0.0f } } },
	{ { {  1.0f, -1.0f, -1.0f, 0.0f } } },
	{ { {  1.0f,  1.0f, -1.0f, 0.0f } } },
	{ { { -1.0f,  1.0f, -1.0f, 0.0f } } },
};

//-----------------------------
// BoundingFrustom
//-----------------------------
//_Use_decl_annotations_
inline void BoundingMath::BoundingFrustum::build(float horFV, float aspectRatio, float n, float f)
{
	FocalLength = 1.0f / (aspectRatio * tan(horFV / 2.0f));
	AspectRatio = aspectRatio;
	Near = n; 
	Far = f;

	buildPlanes();
}

_Use_decl_annotations_
inline void BoundingMath::BoundingFrustum::buildPlanes()
{
	/*
		It looks like we build OUTWARD Normal plane vectors, not INWARDS !!!		
	*/
	
	using namespace DirectX;
	NearPlane = XMFLOAT4(0, 0, -1, Near);
	FarPlane= XMFLOAT4(0, 0, 1, -Far);

	float e2_1 = sqrtf(FocalLength*FocalLength + 1);
	float e2_a = sqrtf(FocalLength*FocalLength + AspectRatio* AspectRatio);

	LeftPlane = XMFLOAT4(-FocalLength/ e2_1, 0, -1/ e2_1, 0);
	RightPlane = XMFLOAT4(FocalLength/ e2_1, 0, -1/ e2_1, 0);
	
	BottomPlane = XMFLOAT4(0, -FocalLength/ e2_a, -AspectRatio/e2_a, 0);
	TopPlane= XMFLOAT4(0, FocalLength/ e2_a, -AspectRatio/e2_a, 0);	
}

inline void BoundingMath::BoundingFrustum::setPlanes(DirectX::XMVECTOR& np, DirectX::XMVECTOR& fp, DirectX::XMVECTOR& lp, DirectX::XMVECTOR& rp,
	DirectX::XMVECTOR& bp, DirectX::XMVECTOR& tp)
{
	DirectX::XMStoreFloat4(&NearPlane, np);
	DirectX::XMStoreFloat4(&FarPlane, fp);
	DirectX::XMStoreFloat4(&LeftPlane, lp);
	DirectX::XMStoreFloat4(&RightPlane, rp);
	DirectX::XMStoreFloat4(&BottomPlane, bp);
	DirectX::XMStoreFloat4(&TopPlane, tp);
}

inline bool BoundingMath::BoundingFrustum::intersects(const BoundingSphere& bb)
{
	using namespace DirectX;
	
	XMVECTOR Q = XMLoadFloat4(&bb.Center);
	XMVECTOR lL = XMLoadFloat4(&NearPlane);

	float lDist[6];
	lDist[0] = XMVectorGetX(XMVector4Dot(Q, lL));
	bool nearTest = lDist[0] > bb.R;

	lL = XMLoadFloat4(&FarPlane);
	lDist[1] = XMVectorGetX(XMVector4Dot(Q, lL));
	bool farTest = lDist[1] > bb.R;

	lL = XMLoadFloat4(&LeftPlane);
	lDist[2] = XMVectorGetX(XMVector4Dot(Q, lL));
	bool leftTest = lDist[2] > bb.R;
	
	lL = XMLoadFloat4(&RightPlane);
	lDist[3] = XMVectorGetX(XMVector4Dot(Q, lL));
	bool rightTest = lDist[3] > bb.R;
	
	lL = XMLoadFloat4(&BottomPlane);
	lDist[4] = XMVectorGetX(XMVector4Dot(Q, lL));
	bool bottomTest = lDist[4] > bb.R;
	
	lL = XMLoadFloat4(&TopPlane);
	lDist[5] = XMVectorGetX(XMVector4Dot(Q, lL));
	bool topTest = lDist[5] > bb.R;

	return !(farTest | nearTest | leftTest | rightTest | bottomTest | topTest);
}

inline bool BoundingMath::BoundingFrustum::intersects(const BoundingBox& bb)
{
	//return TRUE if does intersect

	using namespace DirectX;	

	//TO_DO: maybe do in this way this?
	/*XMVECTOR vNear = XMLoadFloat4(&NearPlane);
	XMVECTOR vFar = XMLoadFloat4(&FarPlane);
	XMVECTOR vLeft= XMLoadFloat4(&LeftPlane);
	XMVECTOR vRight= XMLoadFloat4(&RightPlane);
	XMVECTOR vBottom= XMLoadFloat4(&BottomPlane);
	XMVECTOR vTop= XMLoadFloat4(&TopPlane);*/
	
	sNearPlane = XMLoadFloat4(&NearPlane);
	sFarPlane = XMLoadFloat4(&FarPlane);
	sLeftPlane = XMLoadFloat4(&LeftPlane);
	sRightPlane = XMLoadFloat4(&RightPlane);
	sBottomPlane = XMLoadFloat4(&BottomPlane);
	sTopPlane = XMLoadFloat4(&TopPlane); 

	EffR1 = bb.getEffectiveRadius(sNearPlane);
	EffR2 = bb.getEffectiveRadius(sLeftPlane);
	EffR3 = bb.getEffectiveRadius(sRightPlane);
	EffR4 = bb.getEffectiveRadius(sBottomPlane);
	EffR5 = bb.getEffectiveRadius(sTopPlane);

	XMVECTOR Q = XMLoadFloat4(&bb.Center);
	float lDist[6];

	lDist[0] = XMVectorGetX(XMVector4Dot(Q, sNearPlane));
	bool nearTest = lDist[0] > EffR1;	
	if (nearTest) return false;

	lDist[1] = XMVectorGetX(XMVector4Dot(Q, sFarPlane));
	bool farTest = lDist[1] > EffR1;
	if (farTest) return false;
		
	lDist[2] = XMVectorGetX(XMVector4Dot(Q, sLeftPlane));
	bool leftTest = lDist[2] > EffR2;
	if (leftTest) return false;
		
	lDist[3] = XMVectorGetX(XMVector4Dot(Q, sRightPlane));
	bool rightTest = lDist[3] > EffR3;
	if (rightTest) return false;
		
	lDist[4] = XMVectorGetX(XMVector4Dot(Q, sBottomPlane));
	bool bottomTest = lDist[4] > EffR4;
	if (bottomTest) return false;
		
	lDist[5] = XMVectorGetX(XMVector4Dot(Q, sTopPlane));
	bool topTest = lDist[5] > EffR5;
	if (topTest) return false;

	//return !(farTest | nearTest | leftTest | rightTest | bottomTest | topTest);
	return true;
}

inline bool BoundingMath::BoundingFrustum::intersects(DirectX::XMFLOAT4& box_center, float r1, float r2, float r3, float r4, float r5)
{
	//return TRUE if does intersect

	using namespace DirectX;
	
	XMVECTOR Q = XMLoadFloat4(&box_center);
	float lDist[6];

	lDist[0] = XMVectorGetX(XMVector4Dot(Q, sNearPlane));
	bool nearTest = lDist[0] > r1;
	if (nearTest) return false;

	lDist[1] = XMVectorGetX(XMVector4Dot(Q, sFarPlane));
	bool farTest = lDist[1] > r1;
	if (farTest) return false;

	lDist[2] = XMVectorGetX(XMVector4Dot(Q, sLeftPlane));
	bool leftTest = lDist[2] > r2;
	if (leftTest) return false;

	lDist[3] = XMVectorGetX(XMVector4Dot(Q, sRightPlane));
	bool rightTest = lDist[3] > r3;
	if (rightTest) return false;

	lDist[4] = XMVectorGetX(XMVector4Dot(Q, sBottomPlane));
	bool bottomTest = lDist[4] > r4;
	if (bottomTest) return false;

	lDist[5] = XMVectorGetX(XMVector4Dot(Q, sTopPlane));
	bool topTest = lDist[5] > r5;
	if (topTest) return false;

	//return !(farTest | nearTest | leftTest | rightTest | bottomTest | topTest);
	return true;
}

inline BoundingMath::ContainmentType BoundingMath::BoundingFrustum::contains
(
	DirectX::XMFLOAT4& box_center, float r1, float r2, float r3, float r4, float r5
)
{
	//return TRUE if does intersect

	using namespace DirectX;

	XMVECTOR Q = XMLoadFloat4(&box_center);
	float lDist[6];

	lDist[0] = XMVectorGetX(XMVector4Dot(Q, sNearPlane));
	lDist[1] = XMVectorGetX(XMVector4Dot(Q, sFarPlane));
	lDist[2] = XMVectorGetX(XMVector4Dot(Q, sLeftPlane));
	lDist[3] = XMVectorGetX(XMVector4Dot(Q, sRightPlane));
	lDist[4] = XMVectorGetX(XMVector4Dot(Q, sBottomPlane));
	lDist[5] = XMVectorGetX(XMVector4Dot(Q, sTopPlane));	
	
	// DISJOINT test
	bool nearTest = lDist[0] > r1;
	bool farTest = lDist[1] > r1;
	bool leftTest = lDist[2] > r2;
	bool rightTest = lDist[3] > r3;
	bool bottomTest = lDist[4] > r4;
	bool topTest = lDist[5] > r5;

	bool lResult = (farTest | nearTest | leftTest | rightTest | bottomTest | topTest);
	
	if (lResult) return BoundingMath::ContainmentType::DISJOINT;

	// CONTAINS test
	nearTest = lDist[0] <= -r1;
	farTest = lDist[1] <= -r1;
	leftTest = lDist[2] <= -r2;
	rightTest = lDist[3] <= -r3;
	bottomTest = lDist[4] <= -r4;
	topTest = lDist[5] <= -r5;

	lResult = (farTest && nearTest && leftTest && rightTest && bottomTest && topTest);
	if (lResult) return BoundingMath::ContainmentType::CONTAINS;
	
	return BoundingMath::ContainmentType::INTERSECTS;
}

inline void BoundingMath::BoundingFrustum::transform(BoundingFrustum& fr, DirectX::XMMATRIX& m)
{
	using namespace DirectX;
	XMVECTOR nearV = XMLoadFloat4(&NearPlane);
	XMVECTOR farV = XMLoadFloat4(&FarPlane);
	XMVECTOR leftV = XMLoadFloat4(&LeftPlane);
	XMVECTOR rightV = XMLoadFloat4(&RightPlane);
	XMVECTOR bottomV = XMLoadFloat4(&BottomPlane);
	XMVECTOR topV = XMLoadFloat4(&TopPlane);	
		
	nearV = XMVector4Transform(nearV, m);
	farV = XMVector4Transform(farV, m);
	leftV = XMVector4Transform(leftV, m);
	rightV = XMVector4Transform(rightV, m);
	bottomV = XMVector4Transform(bottomV, m);
	topV = XMVector4Transform(topV, m);

	nearV = XMPlaneNormalize(nearV);
	farV = XMPlaneNormalize(farV);
	leftV = XMPlaneNormalize(leftV);
	rightV = XMPlaneNormalize(rightV);
	bottomV = XMPlaneNormalize(bottomV);
	topV = XMPlaneNormalize(topV);

	fr.setPlanes(nearV, farV, leftV, rightV, bottomV, topV);
}

inline void BoundingMath::BoundingFrustum::getPlanesFromMatrix(BoundingFrustum& fr, DirectX::XMMATRIX& m)
{
	using namespace DirectX;
	
	XMFLOAT4X4 cb;
	XMStoreFloat4x4(&cb, m);
	
	//left
	XMFLOAT4 lp ;	
	lp.x = cb._14 + cb._11;
	lp.y = cb._24 + cb._21;
	lp.z = cb._34 + cb._31;
	lp.w = cb._44 + cb._41;

	//right
	XMFLOAT4 rp;
	rp.x = cb._14 - cb._11;
	rp.y = cb._24 - cb._21;
	rp.z = cb._34 - cb._31;
	rp.w = cb._44 - cb._41;

	//top
	XMFLOAT4 tp;
	tp.x = cb._14 - cb._12;
	tp.y = cb._24 - cb._22;
	tp.z = cb._34 - cb._32;
	tp.w = cb._44 - cb._42;

	//bottom
	XMFLOAT4 bp;
	bp.x = cb._14 + cb._12;
	bp.y = cb._24 + cb._22;
	bp.z = cb._34 + cb._32;
	bp.w = cb._44 + cb._42;

	//near
	XMFLOAT4 np;
	np.x = cb._13;
	np.y = cb._23;
	np.z = cb._33;
	np.w = cb._43;

	//far
	XMFLOAT4 fp;
	fp.x = cb._14 - cb._13;
	fp.y = cb._24 - cb._23;
	fp.z = cb._34 - cb._33;
	fp.w = cb._44 - cb._43;	

	
	XMVECTOR koef = XMVectorSet(-1.0f, -1.0f, -1.0f, -1.0f);
	XMVECTOR nearV = XMLoadFloat4(&np);
	XMVECTOR farV = XMLoadFloat4(&fp);
	XMVECTOR leftV = XMLoadFloat4(&lp);
	XMVECTOR rightV = XMLoadFloat4(&rp);
	XMVECTOR bottomV = XMLoadFloat4(&bp);
	XMVECTOR topV = XMLoadFloat4(&tp);

	nearV *= koef;
	farV *= koef;
	leftV *= koef;
	rightV *= koef;
	bottomV *= koef;
	topV *= koef;

	nearV = XMPlaneNormalize(nearV);
	farV = XMPlaneNormalize(farV);
	leftV= XMPlaneNormalize(leftV);
	rightV = XMPlaneNormalize(rightV);
	bottomV = XMPlaneNormalize(bottomV);
	topV = XMPlaneNormalize(topV);

	fr.setPlanes(nearV, farV, leftV, rightV, bottomV, topV);
}


//-----------------------------
// BoundingCircle
//-----------------------------

inline void BoundingMath::BoundingSphere::build(float r, DirectX::XMFLOAT3 center)
{
	R = r;	
	Center = DirectX::XMFLOAT4(center.x, center.y, center.z, 1.0f);

	basis_R = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
	basis_S = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
	basis_T = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
}

//-----------------------------
// BoundingBox
//-----------------------------

inline BoundingMath::BoundingBox::BoundingBox(const DirectX::BoundingBox& bb)
{
	build(bb.Center, bb.Extents);
}

inline BoundingMath::BoundingBox::BoundingBox(const BoundingMath::BoundingBox& bb)
{
	build(bb.Center, bb.Extents);
}

inline void BoundingMath::BoundingBox::build(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extents)
{
	build(DirectX::XMFLOAT4(center.x, center.y, center.z, 1.0f), extents);	
}

inline void BoundingMath::BoundingBox::build(DirectX::XMFLOAT4 center, DirectX::XMFLOAT3 extents)
{
	Center = center;
	Extents = extents;

	basis_R = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
	basis_S = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
	basis_T = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);

	basis_R.x *= Extents.x * 2; // base should have we all lenght of bounding box side, Extent is Radius, i.e. only half
	basis_S.y *= Extents.y * 2;
	basis_T.z *= Extents.z * 2;
}

inline float BoundingMath::BoundingBox::getEffectiveRadius(const DirectX::XMVECTOR& L) const
{	
	using namespace DirectX;

	DirectX::XMVECTOR R = XMLoadFloat3(&basis_R);
	DirectX::XMVECTOR S = XMLoadFloat3(&basis_S);
	DirectX::XMVECTOR T = XMLoadFloat3(&basis_T);	

	float Rdot = XMVectorGetX(XMVector3Dot(R, L));
	float Sdot = XMVectorGetX(XMVector3Dot(S, L));
	float Tdot = XMVectorGetX(XMVector3Dot(T, L));

	float rEff = 0.5 * (fabsf(Rdot) + fabsf(Sdot) + fabsf(Tdot));

	return rEff;
}

inline void BoundingMath::BoundingBox::CreateMerged(BoundingBox& out, const BoundingBox& in1, const BoundingBox& in2)
{
	using namespace DirectX;
	XMVECTOR b1Center = XMLoadFloat4(&in1.Center);
	XMVECTOR b1Extents = XMLoadFloat3(&in1.Extents);

	XMVECTOR b2Center = XMLoadFloat4(&in2.Center);
	XMVECTOR b2Extents = XMLoadFloat3(&in2.Extents);

	XMVECTOR Min = XMVectorSubtract(b1Center, b1Extents);
	Min = XMVectorMin(Min, XMVectorSubtract(b2Center, b2Extents));

	XMVECTOR Max = XMVectorAdd(b1Center, b1Extents);
	Max = XMVectorMax(Max, XMVectorAdd(b2Center, b2Extents));

	assert(XMVector3LessOrEqual(Min, Max));

	XMStoreFloat4(&out.Center, XMVectorScale(XMVectorAdd(Min, Max), 0.5f));
	XMStoreFloat3(&out.Extents, XMVectorScale(XMVectorSubtract(Max, Min), 0.5f));
}


inline void BoundingMath::BoundingBox::GetCorners(DirectX::XMFLOAT3* Corners) const
{
	assert(Corners != nullptr);

	// Load the box
	DirectX::XMVECTOR vCenter = XMLoadFloat4(&Center);
	DirectX::XMVECTOR vExtents = XMLoadFloat3(&Extents);

	for (size_t i = 0; i < CORNER_COUNT; ++i)
	{
		DirectX::XMVECTOR C = DirectX::XMVectorMultiplyAdd(vExtents, g_BoxOffset[i], vCenter);
		XMStoreFloat3(&Corners[i], C);
	}
}

inline BoundingMath::ContainmentType BoundingMath::BoundingBox::contains(BoundingBox& bb)
{
	using namespace DirectX;
	XMVECTOR center1 = XMLoadFloat4(&Center);
	XMVECTOR center2 = XMLoadFloat4(&bb.Center);

	XMVECTOR extents1 = XMLoadFloat3(&Extents);
	XMVECTOR extents2 = XMLoadFloat3(&bb.Extents);

	XMVECTOR lSize = XMVectorAbs(center2 - center1) ;
	XMVECTOR lSize2 = lSize + extents2;

	XMVECTOR lResult = XMVectorGreater(lSize, extents1 + extents2);

	if (Internal::XMVector3AnyTrue(lResult))
		return BoundingMath::ContainmentType::DISJOINT;

	lResult = XMVectorLessOrEqual(lSize2, extents1);
	if (Internal::XMVector3AllTrue(lResult))
		return BoundingMath::ContainmentType::CONTAINS;

	return ContainmentType::INTERSECTS;
}