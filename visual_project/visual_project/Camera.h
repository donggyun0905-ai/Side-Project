// Camera.h
#pragma once

#include <GL/glew.h>
#include <GL/freeglut.h>

class Camera {
public:
    float radius;
    float azimuth;    // 수평 각
    float elevation;  // 수직 각

    Camera();

    void applyView() const;  // gluLookAt 적용
};
