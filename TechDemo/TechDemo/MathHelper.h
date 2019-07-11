#pragma once
//#include "DirectXMath.h"
#include <DirectXCollision.h>

#define RAND_MAX 0x7fff

class MathHelper {
public:
	static DirectX::XMFLOAT4X4 Identity4x4()
	{
		static DirectX::XMFLOAT4X4 I(
			1.0f, 0, 0, 0,
			0, 1.0f, 0, 0,
			0, 0, 1.0f, 0,
			0, 0, 0, 1.0f);
		return I;
	}
	
	// Returns random float in [0, 1).
	static float RandF()
	{
		return (float)(rand()) / (float)RAND_MAX;
	}

	// Returns random float in [a, b).
	static float RandF(float a, float b)
	{
		return a + RandF()*(b - a);
	}

	static void buildSunOrthoLightProjection(DirectX::XMFLOAT3& lightDirection, DirectX::XMFLOAT4X4& lightViewProj, 
		DirectX::XMFLOAT4X4& lightViewProjT,		
		DirectX::BoundingSphere sceneBS)
	{
		using namespace DirectX;
		XMVECTOR lLightDirection = XMLoadFloat3(&lightDirection);
		XMVECTOR lLightTarget = XMLoadFloat3(&sceneBS.Center);
		XMVECTOR lLightPosition = lLightTarget - 1.0f * sceneBS.Radius *lLightDirection;
		XMVECTOR lLightUpVector = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		XMMATRIX lLightView = XMMatrixLookAtLH(lLightPosition, lLightTarget, lLightUpVector);

		XMFLOAT3 lSphereCenterLightSpace;
		XMStoreFloat3(&lSphereCenterLightSpace, XMVector3TransformCoord(lLightTarget, lLightView));

		float l = lSphereCenterLightSpace.x - sceneBS.Radius;
		float r = lSphereCenterLightSpace.x + sceneBS.Radius;

		float b = lSphereCenterLightSpace.y - sceneBS.Radius;
		float t = lSphereCenterLightSpace.y + sceneBS.Radius;

		float n = lSphereCenterLightSpace.z - sceneBS.Radius;
		float f = lSphereCenterLightSpace.z + sceneBS.Radius;

		XMMATRIX lLightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

		XMMATRIX T(
			0.5, 0.0f, 0.0f, 0.0f,
			0.0, -0.5f, 0.0f, 0.0f,
			0.0, 0.0f, 1.0f, 0.0f,
			0.5, 0.5f, 0.0f, 1.0f);

		XMMATRIX S = lLightView * lLightProj;
		XMStoreFloat4x4(&lightViewProj, XMMatrixTranspose(S));
		S *= T;
		XMStoreFloat4x4(&lightViewProjT, XMMatrixTranspose(S));
	}
};