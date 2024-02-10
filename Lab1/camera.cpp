#include "camera.h"

bool Camera::updateRadius(float offsetR)
{
    float newCameraR = cameraR + offsetR;
    if (newCameraR > minScaleSize && newCameraR < maxScaleSize) {
        cameraR = newCameraR;
        return true;
    }
    return false;
}

void Camera::updateTarget(float offsetX, float offsetY, float offsetZ)
{
    target.x += offsetX;
    target.y += offsetY;
    target.z += offsetZ;
}

void Camera::updateViewMatrix()
{
    eye.x = cameraR * cos(cameraPhi) * sin(cameraZeta);
    eye.z = cameraR * sin(cameraPhi) * sin(cameraZeta);
    eye.y = cameraR * cos(cameraZeta);
    view(eye, target, up);
    front = target - eye;
}
