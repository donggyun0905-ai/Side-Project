// Drone.cpp
#include "Drone.h"
#include <cmath>

Drone::Drone()
    : position(0.0f, 0.0f, 0.0f),
    target(0.0f, 0.0f, 0.0f),
    color(0.0f, 0.0f, 1.0f)
{
}

Drone::Drone(const Vec3& pos, const Vec3& col)
    : position(pos),
    target(pos),   // 처음에는 position과 동일하게
    color(col)
{
}

void Drone::update(float dt, const std::vector<Drone>& all)
{
    // ===============================
    // 0️⃣ 도착 판정 (⭐ 핵심)
    // ===============================
    Vec3 diff(
        target.x - position.x,
        target.y - position.y,
        target.z - position.z
    );

    float distToTarget = std::sqrt(
        diff.x * diff.x +
        diff.y * diff.y +
        diff.z * diff.z
    );

    const float ARRIVE_DIST = 0.03f;

    if (distToTarget < ARRIVE_DIST)
    {
        // 정확히 고정
        position = target;
        return; // 🔒 이후 force 계산 전부 차단
    }

    // ===============================
    // 1️⃣ target 방향
    // ===============================
    float speed = 2.0f;

    Vec3 dir = diff;
    if (distToTarget > 0.0001f) {
        dir.x /= distToTarget;
        dir.y /= distToTarget;
        dir.z /= distToTarget;
    }

    // ===============================
    // 2️⃣ 거리 유지 (회피)
    // ===============================
    float minDist = 0.2f;          // ⭐ 0이면 의미 없음 → 반드시 양수
    float avoidStrength = 0.1f;

    Vec3 avoid(0, 0, 0);

    for (const auto& other : all) {
        if (&other == this) continue;

        float dx = position.x - other.position.x;
        float dy = position.y - other.position.y;
        float dz = position.z - other.position.z;

        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist < minDist && dist > 0.0001f) {
            avoid.x += dx / dist;
            avoid.y += dy / dist;
            avoid.z += dz / dist;
        }
    }

    float alen = std::sqrt(
        avoid.x * avoid.x +
        avoid.y * avoid.y +
        avoid.z * avoid.z
    );

    if (alen > 0.001f) {
        avoid.x = (avoid.x / alen) * avoidStrength;
        avoid.y = (avoid.y / alen) * avoidStrength;
        avoid.z = (avoid.z / alen) * avoidStrength;
    }

    // ===============================
    // 3️⃣ 최종 이동
    // ===============================
    Vec3 move(
        dir.x + avoid.x,
        dir.y + avoid.y,
        dir.z + avoid.z
    );

    float mlen = std::sqrt(
        move.x * move.x +
        move.y * move.y +
        move.z * move.z
    );

    if (mlen > 0.0001f) {
        move.x /= mlen;
        move.y /= mlen;
        move.z /= mlen;
    }

    // ===============================
    // 4️⃣ 위치 업데이트
    // ===============================
    position.x += move.x * speed * dt;
    position.y += move.y * speed * dt;
    position.z += move.z * speed * dt;
}


void Drone::draw() const
{

    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glutSolidSphere(0.1, 12, 12);
    glPopMatrix();
}