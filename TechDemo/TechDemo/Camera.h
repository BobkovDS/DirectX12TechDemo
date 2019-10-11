/*
	***************************************************************************************************
	Description:
		Camera class with two Camera Lens classes. Provides general methods to Set and Transform a camera. Also provides View matrix 
		with BoundingFrustum object in World space for Frustum Culling. Camera lens class provides Projection matrix. 
		
	***************************************************************************************************
*/

#pragma once

#include <vector>
#include "MathHelper.h"
#include "Executer.h"
#include "BoundingMath.h"

class Camera{
	
	DirectX::XMFLOAT3 m_position;	
	DirectX::XMFLOAT3 m_right;
	DirectX::XMFLOAT3 m_up;
	DirectX::XMFLOAT3 m_look;		
	
	bool m_viewToUpdate; //Need to rebuild a view matrix
	bool m_boundingFrustumWorldToUpdate;		
	DirectX::XMFLOAT4X4 m_viewMatrix;	
	BoundingMath::BoundingFrustum m_boundingFrustumCameraWorld; //Frustum for Camera in World space			
	std::vector<ExecuterBase*> m_observers;	
	void updateObservers();
	Camera(Camera& a) {};
public:
	Camera();
	~Camera();
	
	Camera& operator=(Camera&);
	class CameraLens;
	CameraLens* lens;
	int animationId;

	//Get/Set world camera position
	DirectX::XMVECTOR getPosition() const;
	const DirectX::XMFLOAT3& getPosition3f();
	void setPosition(float x, float y, float z);
	void setPosition(const DirectX::XMFLOAT3& v);
	//Get camera basic vectors
	DirectX::XMVECTOR getRight() const;
	const DirectX::XMFLOAT3& getRight3f();
	DirectX::XMVECTOR getUp() const;
	const DirectX::XMFLOAT3& getUp3f();
	DirectX::XMVECTOR getLook() const;
	const DirectX::XMFLOAT3& getLook3f();
	DirectX::XMFLOAT3 getTarget3f();

	void addObserver(ExecuterBase* newObserver) { m_observers.push_back(newObserver); }

	//Define camera space via LookAt parametrs
	void lookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void lookAt(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 target, DirectX::XMFLOAT3 worldUp);
	void lookTo(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR lookDir, DirectX::FXMVECTOR worldUp);
	//Update/Get View matrices
	DirectX::XMMATRIX getView();	
	void updateViewMatrix();
	const DirectX::XMFLOAT4X4& getView4x4f();

	//Get Projection matrix	
	const DirectX::XMFLOAT4X4& getProj4x4f();

	// Camera moving functions
	void strafe(float d);
	void walk(float d);
	void pitch(float angle);
	void rotateY(float angle);
	void transform(DirectX::XMFLOAT4X4& transformM);

	// Bounding	Frustum
	BoundingMath::BoundingFrustum& getBoundingFrustomCameraWorld();	

	//Internal base Lens class 
	class CameraLens {		
		CameraLens(const CameraLens&) = delete;		
		
	protected:
		DirectX::XMFLOAT4X4 m_projectionMatrix;		
		bool m_lensWasSet;
	public:
		CameraLens():m_lensWasSet(false) { m_projectionMatrix = {};};
		DirectX::XMMATRIX getProj() const;
		const DirectX::XMFLOAT4X4& getProj4x4f(); // const here it is be cause we use reference, we do not want to change proj matrix outside

		void operator=(const CameraLens& c) 
		{ 
			_copy(&c, this);
		};
		virtual void _copy(const CameraLens* _in, CameraLens* _out) = 0;
		virtual void setLens(float p1, float p2, float p3, float p4)=0;	
		virtual void setLens(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extents)=0;
		virtual ~CameraLens() = 0 {}
	};
};

class PerspectiveCameraLens : public Camera::CameraLens {
	float m_nearZ;
	float m_farZ;
	float m_aspect;
	float m_fovY;
	float m_nearWindowHeight;
	float m_farWindowHeight;	
	PerspectiveCameraLens& operator=(const PerspectiveCameraLens& c) = delete;
public:
	PerspectiveCameraLens();
	
	void _copy(const CameraLens* _in, CameraLens* _out) ;
	void setLens(float fovY, float aspect, float zn, float zf);	
	void setLens(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extents) {};
	void setAspectRatio(float aspectRatio);	
};

class OrthographicCameraLesn : public Camera::CameraLens {
public:
	OrthographicCameraLesn() {};
	OrthographicCameraLesn& operator=(const OrthographicCameraLesn& c);
	void setLens(float p1, float p2, float p3, float p4);
	void setLens(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extents);
};

// ----- Inline Functions -------

inline DirectX::XMVECTOR Camera::getPosition() const
{
	return XMLoadFloat3(&m_position);
}

inline DirectX::XMVECTOR Camera::getLook() const
{
	return DirectX::XMLoadFloat3(&m_look);
}

inline const DirectX::XMFLOAT4X4& Camera::getView4x4f()
{
	return m_viewMatrix;
}