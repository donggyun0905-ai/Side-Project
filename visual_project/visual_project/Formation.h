// Formation.h
#pragma once

#include <vector>
#include <string>
#include "Drone.h"

struct DronePose {
    Vec3 pos;          // raw position from CSV
    Vec3 color;        // fixed color from CSV
    DronePart part;    // ✅ PART 추가
};

class Formation {
public:
    static void setCircle(std::vector<Drone>& drones, float radius);
    static void setGrid(std::vector<Drone>& drones, int cols, float spacing, Vec3 start = Vec3(0, 0, 0));

    static void setTextTwoLines(
        std::vector<Drone>& drones,
        const std::string& line1,
        const std::string& line2,
        float spacing,
        float lineGap,
        Vec3 offset
    );

    static bool setFromCSV(
        std::vector<Drone>& drones,
        const std::string& csvPath,
        float scale = 1.0f,
        Vec3 offset = Vec3(0, 0, 0),
        bool autoCenter = false,
        bool swapYZ = false,
        bool overrideColor = true
    );

    static bool setFromCSV_FitStage(
        std::vector<Drone>& drones,
        const std::string& csvPath,
        Vec3 stageCenter = Vec3(0, 2.0f, 0),
        float stageHalfSize = 8.0f,
        bool swapYZ = false,
        bool overrideColor = true
    );

    static void setWinterDream(std::vector<Drone>& drones); 
    static void setIntroFromCSV(std::vector<Drone>& drones); // snow.csv
    static void setSanta(std::vector<Drone>& drones);
    static void setElsa(std::vector<Drone>& drones);
    static void setAnna(std::vector<Drone>& drones);
    static void setOlaf(std::vector<Drone>& drones);
};
