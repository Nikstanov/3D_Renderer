#pragma once
#include "geometry.h"
#include "engine.h"

class Camera {
private:
	Vec3f eye{ 0.0f, 2.f, 2.0f };
	Vec3f up{ 0.0f , 1.0f , 0.0f };
	Vec3f target{ 0.0f , 1.0f , 0.0f };
	Vec3f front;
	
	float cameraR = 5.0f;
	float minScaleSize = 0.0001f;
	float maxScaleSize = cameraR * 3.0f;

public:
	float cameraZeta = 3.141557f / 2;
	float cameraPhi = 3.141557f / 2;

	bool updateRadius(float offsetR);
	void updateTarget(float offsetX, float offsetY, float offsetZ);
	void updateViewMatrix();
	Camera(float R, float phi, float zeta, Vec3f _target) : cameraR(R), cameraPhi(phi), cameraZeta(zeta), target(_target) { maxScaleSize = cameraR * 3.0f; }
	Vec3f getEye() { return eye; };
	Vec3f getUp() { return up; };
	Vec3f getTarget() { return target; };
	Vec3f getFront() { return front; };
};
