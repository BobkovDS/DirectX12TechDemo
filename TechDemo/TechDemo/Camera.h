#pragma once

#include <vector>
#include "MathHelper.h"
#include "Executer.h"

class Camera{
	DirectX::XMFLOAT3 m_position;	
	DirectX::XMFLOAT3 m_right;
	DirectX::XMFLOAT3 m_up;
	DirectX::XMFLOAT3 m_look;
	DirectX::XMFLOAT4X4 m_localTransformation;
	//bool m_viewDirty; // We updated a View matrix, need to say to all about it
	
	bool m_viewToUpdate; //Need to rebuild a view matrix
	bool m_frustumBoundingWorldToUpdate;
	DirectX::XMFLOAT4X4 m_viewMatrix;
	DirectX::BoundingFrustum m_frustumBounding;
	DirectX::BoundingFrustum m_frustumBoundingWorld;
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

	void setLocalTransformation(DirectX::XMFLOAT4X4& lcTransformation);
	const DirectX::XMFLOAT4X4& getLocalTransformation();
	DirectX::XMMATRIX getLocalTransformationMatrix();

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
	//DirectX::XMMATRIX getProj();	
	const DirectX::XMFLOAT4X4& getProj4x4f();

	// Camera moving functions
	void strafe(float d);
	void walk(float d);
	void pitch(float angle);
	void rotateY(float angle);
	void transform(DirectX::XMFLOAT4X4& transformM);

	//Frustum Bounding
	DirectX::BoundingFrustum& getFrustomBoundingWorld();
	void buildFrustumBounding();

	//Internal base Lens class 
	class CameraLens {		
		CameraLens(const CameraLens&) = delete;		
		
	protected:
		DirectX::XMFLOAT4X4 m_projectionMatrix;
		bool m_lensWasSet;
	public:
		CameraLens():m_lensWasSet(false) { m_projectionMatrix = {};};
		DirectX::XMMATRIX getProj() const;
		const DirectX::XMFLOAT4X4& getProj4x4f(); // const here is be cause we use reference, we do not want to change proj matrix outside

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