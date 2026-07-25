// Controller.cpp
#include "Controller.h"
#include <GL/freeglut.h>

Controller::Controller()
    : camera(nullptr) {}

void Controller::attachCamera(Camera* cam) {
    camera = cam;
}

void Controller::onSpecialKey(int key) {
    if (!camera) return;

    float dTheta = 0.05f;
    float dPhi = 0.05f;

    switch (key) {
    case GLUT_KEY_LEFT:  camera->azimuth -= dTheta; break;
    case GLUT_KEY_RIGHT: camera->azimuth += dTheta; break;
    case GLUT_KEY_UP:    camera->elevation += dPhi;   break;
    case GLUT_KEY_DOWN:  camera->elevation -= dPhi;   break;
    default: break;
    }
}

void Controller::onKey(unsigned char key) {
    if (!camera) return;

    // 기존 zoom 기능
    if (key == '9') camera->radius -= 0.3f;   // 가까워짐
    if (key == '0') camera->radius += 0.3f;   // 멀어짐

    // ---- 여기서부터 새 기능 ----
    if (key == '-') camera->radius += 0.7f;   // - = Zoom OUT (멀어짐)
    if (key == '+') camera->radius -= 0.7f;   // + = Zoom IN  (가까워짐)

    // 최소 거리 제한
    if (camera->radius < 2.0f)
        camera->radius = 2.0f;
}
