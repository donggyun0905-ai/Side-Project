// ShowControl.cpp
#include "ShowControl.h"
#include "Scene.h"
#include "Formation.h"

ShowControl::ShowControl()
    : time(0.0f), textFormed(false) {
    for (int i = 0; i < 5; ++i)
        groupMoved[i] = false;
}

void ShowControl::update(float dt, Scene& scene)
{
    time += dt;

    float delay = 1.0f;
    int groupSize = 49;
    float spacing = 0.7f;

    for (int g = 0; g < 5; g++) {

        if (time > g * delay && !groupMoved[g]) {

            groupMoved[g] = true;

            Vec3 centerOffset(-6.0f, 3.0f, -6.0f);

            for (int i = 0; i < groupSize; i++) {

                int idx = g * groupSize + i;
                if (idx >= (int)scene.drones.size()) break;

                int r = i / 7;
                int c = i % 7;

                float x = centerOffset.x + c * spacing;
                float y = centerOffset.y - g * 1.5f;
                float z = centerOffset.z + r * spacing;

                scene.drones[idx].target = Vec3(x, y, z);
            }
        }
    }

}
