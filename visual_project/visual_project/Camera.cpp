// Camera.cpp
#include "Camera.h"
#include <cmath>

Camera::Camera()
    : radius(25.0f), azimuth(0.0f), elevation(0.3f) {}

void Camera::applyView() const {
    float x = radius * std::cos(elevation) * std::sin(azimuth);
    float y = radius * std::sin(elevation);
    float z = radius * std::cos(elevation) * std::cos(azimuth);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(x, y, z,  // 카메라 위치
        0.0, 0.0, 0.0,  // 바라보는 점
        0.0, 1.0, 0.0); // up vector
}
