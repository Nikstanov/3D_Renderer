#pragma once
#include "geometry.h"
#include "engine.h"

class Camera {
private:
	Vec4f eye{ 0.0f, 2.f, 2.0f , 1};
	Vec4f up{ 0.0f , 1.0f , 0.0f , 1};
	Vec4f target{ 0.0f , 1.0f , 0.0f , 1};
	Vec4f front;
	
	
	float minScaleSize = 0.0001f;
	float maxScaleSize = cameraR * 3.0f;

public:
	float cameraZeta = 3.141557f / 2;
	float cameraPhi = 3.141557f / 2;
	float cameraR = 5.0f;

	bool updateRadius(float offsetR);
	void updateTarget(float offsetX, float offsetY, float offsetZ);
	void updateViewMatrix();
	Camera(float R, float phi, float zeta, Vec4f _target) : cameraR(R), cameraPhi(phi), cameraZeta(zeta), target(_target) { maxScaleSize = cameraR * 3.0f; }
	Vec4f getEye() { return eye; };
	Vec4f getUp() { return up; };
	Vec4f getTarget() { return target; };
	Vec4f getFront() { return front; };
};
