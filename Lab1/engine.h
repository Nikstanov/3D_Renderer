#pragma once
#include "geometry.h"

extern Matrix ModelView;
extern Matrix Projection;
extern Matrix Viewport;

void viewport(int width, int height);
void projection(float aspect, float FOV, float zfar, float znear);
void view(Vec3f eye, Vec3f target, Vec3f up);



