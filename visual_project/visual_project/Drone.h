 // Drone.h
#pragma once

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>

struct Vec3 {
    float x, y, z;
    Vec3(float xx = 0.0f, float yy = 0.0f, float zz = 0.0f)
        : x(xx), y(yy), z(zz) {
    }

    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }
};

// ✅ PART 구분
enum class DronePart {
    NONE = 0,
    BODY,
    ARM,
    HAND
};

class Drone {
public:
    Vec3 position;
    Vec3 target;
    Vec3 color;

    // ✅ 비트맵/CSV 지정색 고정용
    bool colorOverride = false;
    Vec3 overrideColor = Vec3(1, 1, 1);

    // ✅ santa PART
    DronePart part = DronePart::NONE;

    // ✅ 산타 포즈 완성 기준 좌표(팔/손 흔들 때 기준점)
    Vec3 baseTarget = Vec3(0, 0, 0);
    bool hasBaseTarget = false;

    Drone();
    Drone(const Vec3& pos, const Vec3& col);

    void update(float dt, const std::vector<Drone>& all);
    void draw() const;
};
