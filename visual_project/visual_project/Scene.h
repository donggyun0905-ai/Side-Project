// Scene.h
#pragma once

#include <vector>
#include "Drone.h"
#include "Camera.h"

class ShowControl;

enum class ShowSection {
    INTRO,
    VERSE,
    CHORUS,
    OUTRO,
    END
};

class Scene {
public:
    std::vector<Drone> drones;
    float sparkleIntensity = 0.0f;
    Camera camera;
    GLuint droneShader;
    float targetCameraRadius = 25.0f;
    float timeline = 10.0f;
    ShowSection currentSection = ShowSection::INTRO;

    // ✅ 산타 포즈가 세팅된 시간(타임라인 기준)
    float santaPoseTime = -1.0f;

    Scene();

    void init();
    void update(float dt);
    void render() const;
};
