// Controller.h
#pragma once

#include "Camera.h"

class Controller {
public:
    Camera* camera;  // 카메라를 조작하기 위한 포인터

    Controller();

    void attachCamera(Camera* cam);

    void onSpecialKey(int key);
    void onKey(unsigned char key);
};
