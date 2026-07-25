// Scene.cpp
#include "Scene.h"
#include "Formation.h"
#include "shader_loader.h"
#include "Audio.h"
#include <glm/glm.hpp>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

struct HSV {
    float h;
    float s;
    float v;
};

static glm::vec3 HSVtoRGB(const HSV& hsv)
{
    float h = hsv.h * 6.0f;
    int i = (int)floor(h);
    float f = h - i;
    float p = hsv.v * (1.0f - hsv.s);
    float q = hsv.v * (1.0f - f * hsv.s);
    float t = hsv.v * (1.0f - (1.0f - f) * hsv.s);

    switch (i % 6) {
    case 0: return { hsv.v, t, p };
    case 1: return { q, hsv.v, p };
    case 2: return { p, hsv.v, t };
    case 3: return { p, q, hsv.v };
    case 4: return { t, p, hsv.v };
    case 5: return { hsv.v, p, q };
    }
    return { 1,1,1 };
}

AudioAnalyzer gAudio;
float globalTime = 0.0f;
float snowSpinStartTime = -1.0f;
Scene::Scene() {}

void Scene::init() {
    int count = 2000;

    drones.clear();
    drones.reserve(count);

    gAudio.loadAudio("Snow-Princess-Jimena-Contreras.wav", 44100);

    droneShader = LoadShader(
        "shaders/drone_phong.vert",
        "shaders/drone_phong.frag"
    );

    int rows = 7;
    int cols = 7;
    int perFloor = rows * cols;
    int floors = (count + perFloor - 1) / perFloor;

    float spacing = 0.8f;

    Vec3 floorColors[7] = {
        Vec3(1,0,0),
        Vec3(1,0.5,0),
        Vec3(1,1,0),
        Vec3(0,1,0),
        Vec3(0,0.5,1),
        Vec3(0.3,0.7,1),
        Vec3(0.6,0.3,0.8)
    };

    int idx = 0;

    for (int f = 0; f < floors; f++) {
        float baseY = -5.0f - f * 1.5f;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {

                if (idx >= count) break;

                Drone d;

                float x = -10 + c * spacing;
                float y = baseY;
                float z = 0 + r * spacing;

                d.position = d.target = Vec3(x, y, z);
                d.color = floorColors[f % 7];
                d.colorOverride = false;

                // PART/baseTarget 기본값은 Drone.h에서 초기화됨

                drones.push_back(d);
                idx++;
            }
            if (idx >= count) break;
        }
        if (idx >= count) break;
    }

    santaPoseTime = -1.0f;
}

void Scene::update(float dt)
{
    gAudio.update(dt);
    const AudioReactiveData& ar = gAudio.getReactiveData();

    globalTime += dt;

    float low = ar.lowBand;
    float mid = ar.midBand;
    float high = ar.highBand;
    float beat = 0.5f + 0.5f * sinf(globalTime * 6.0f);

    float speedBoost = 1.0f + high * 1.5f + beat * 1.5f;
    speedBoost = glm::clamp(speedBoost, 1.0f, 3.0f);

    HSV hsv;
    hsv.h = fmod(low * 0.8f + 0.55f, 1.0f);
    hsv.s = glm::clamp(0.3f + mid * 0.7f, 0.0f, 1.0f);
    hsv.v = 0.9f;

    glm::vec3 rgb = HSVtoRGB(hsv);

    timeline += dt;
    // ------------------------------------------------------------
// ❄ SNOW 포메이션 도착 후 10초간 회전
// ------------------------------------------------------------
    if (currentSection == ShowSection::VERSE && snowSpinStartTime >= 0.0f)
    {
        float spinDuration = 10.0f;               // ⭐ 10초
        float t = timeline - snowSpinStartTime;

        if (t < spinDuration)
        {
            float omega = glm::two_pi<float>() / spinDuration; // 한 바퀴
            float angle = t * omega;

            // 무대 중심 (Formation에서 쓴 값과 동일)
            Vec3 center(0.0f, 2.0f, 0.0f);

            for (auto& d : drones)
            {
                if (!d.hasBaseTarget) continue;

                // 중심 기준 상대 좌표
                float dx = d.baseTarget.x - center.x;
                float dz = d.baseTarget.z - center.z;

                // Y는 고정, XZ만 회전
                float cosA = cosf(angle);
                float sinA = sinf(angle);

                float rx = dx * cosA - dz * sinA;
                float rz = dx * sinA + dz * cosA;

                d.target = Vec3(
                    center.x + rx,
                    d.baseTarget.y,   // 높이는 고정
                    center.z + rz
                );
            }
        }
        else
        {
            // ✅ 회전 종료 → 원래 포즈로 고정
            for (auto& d : drones)
            {
                if (!d.hasBaseTarget) continue;
                d.target = d.baseTarget;
            }

            // 한 번만 실행되게
            snowSpinStartTime = -1.0f;
        }
    }

    // ------------------------------------------------------------
    // ✅ 산타가 완성된 후(조금 딜레이) ARM/HAND만 "반원 궤적"으로 target을 흔듦
    // - baseTarget은 Formation에서 CSV 적용 시 저장됨
    // - OUTRO 구간이 산타 구간이므로 여기서만 동작
    // ------------------------------------------------------------
    if (currentSection == ShowSection::OUTRO && santaPoseTime >= 0.0f)
    {
        float delayAfterSanta = 2.0f;              // 산타 세팅 후 2초는 가만히(“완성된 후” 느낌)
        float t = timeline - santaPoseTime - delayAfterSanta;

        if (t > 0.0f)
        {
            float omega = 3.0f;                    // 흔드는 속도
            float angleMax = 0.9f;                 // 라디안(약 51도)
            float angle = sinf(t * omega) * angleMax;

            for (auto& d : drones)
            {
                if (!d.hasBaseTarget) continue;
                if (d.part != DronePart::ARM && d.part != DronePart::HAND) continue;

                float radius = (d.part == DronePart::HAND) ? 0.9f : 0.5f; // 손이 더 크게
                // 반원(호) 느낌: YZ 평면에서 원호 이동 (시작점 angle=0 → offset 0)
                float offY = radius * sinf(angle);
                float offZ = radius * (cosf(angle) - 1.0f); // 0에서 시작, 좌우 흔들 때 살짝 앞/뒤로 호가 생김

                d.target = Vec3(d.baseTarget.x,
                    d.baseTarget.y + offY,
                    d.baseTarget.z + offZ);
            }
        }
        else
        {
            // 딜레이 동안은 baseTarget 유지(고정)
            for (auto& d : drones) {
                if (!d.hasBaseTarget) continue;
                if (d.part != DronePart::ARM && d.part != DronePart::HAND) continue;
                d.target = d.baseTarget;
            }
        }
    }

    // ------------------------------------------------------------
    // 기존 색/이동 업데이트 루프 (틀 유지)
    // ------------------------------------------------------------
    for (auto& d : drones)
    {
        if (d.colorOverride) d.color = d.overrideColor;
        else                 d.color = Vec3(rgb.r, rgb.g, rgb.b);

        if (currentSection != ShowSection::CHORUS)
        {
            d.update(dt * speedBoost, drones);
        }
    }

    sparkleIntensity = high * 0.4f + beat * 0.6f;

    // ------------------------------------------------------------
    // 기존 타임라인(틀 유지) + 산타 세팅 시점 기록만 추가
    // ------------------------------------------------------------
    if (currentSection == ShowSection::INTRO)
    {
        if (timeline > 16.0f)
        {
            Formation::setWinterDream(drones);
            targetCameraRadius = 55.0f;
        }

        if (timeline > 35.0f)
        {
            Formation::setIntroFromCSV(drones);
            currentSection = ShowSection::VERSE;
            targetCameraRadius = 55.0f;

            snowSpinStartTime = timeline;
        }
    }
    else if (currentSection == ShowSection::VERSE)
    {
        if (timeline > 50.0f)
        {
            Formation::setSanta(drones);

            // ✅ 산타 포즈 세팅된 시점 기록(이 시간 기준으로 “완성 후” 딜레이/흔들기)
            santaPoseTime = timeline;

            currentSection = ShowSection::OUTRO;
        }
    }
    else if (currentSection == ShowSection::OUTRO)
    {
        if (timeline > 60.0f)
        {
            Formation::setElsa(drones);

            // ✅ 산타 구간 끝났으니 비활성(굳이 없어도 되지만 안전)
            santaPoseTime = -1.0f;
        }

        if (timeline > 70.0f)
        {
            Formation::setAnna(drones);
            currentSection = ShowSection::END;
        }
    }
    else if (currentSection == ShowSection::END)
    {
        if (timeline > 80.0f)
        {
            Formation::setOlaf(drones);
        }
    }

    camera.radius += (targetCameraRadius - camera.radius) * 0.0001f;

    static float tt = 0;
    tt += dt;
    if (tt > 0.3f) {
        std::cout << "MID=" << mid
            << " HIGH=" << high
            << " BEAT=" << beat << std::endl;
        tt = 0;
    }
}

void Scene::render() const
{
    glUseProgram(droneShader);

    float camX = camera.radius * cosf(camera.elevation) * sinf(camera.azimuth);
    float camY = camera.radius * sinf(camera.elevation);
    float camZ = camera.radius * cosf(camera.elevation) * cosf(camera.azimuth);

    glm::vec3 camPos(camX, camY, camZ);

    glm::mat4 view = glm::lookAt(
        camPos,
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0)
    );

    glm::mat4 proj = glm::perspective(
        glm::radians(60.0f),
        800.0f / 600.0f,
        0.1f,
        100.0f
    );

    glUniformMatrix4fv(glGetUniformLocation(droneShader, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(droneShader, "uProj"), 1, GL_FALSE, glm::value_ptr(proj));

    glUniform3f(glGetUniformLocation(droneShader, "viewPos"), camPos.x, camPos.y, camPos.z);
    glUniform3f(glGetUniformLocation(droneShader, "lightPos"), 5.0f, 5.0f, 5.0f);

    glUniform1f(glGetUniformLocation(droneShader, "uSparkleIntensity"), sparkleIntensity);

    for (const auto& d : drones)
    {
        glm::mat4 model = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(d.position.x, d.position.y, d.position.z)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(droneShader, "uModel"),
            1, GL_FALSE,
            glm::value_ptr(model)
        );

        glUniform3f(
            glGetUniformLocation(droneShader, "color"),
            d.color.x, d.color.y, d.color.z
        );

        d.draw();
    }

    glUseProgram(0);

    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(5, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, 5, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 5);
    glEnd();
}
