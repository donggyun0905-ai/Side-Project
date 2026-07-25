// Formation.cpp
#include "Formation.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <iostream>
#include <cmath>

static float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static std::string toLower(std::string s) {
    for (auto& ch : s) ch = (char)std::tolower((unsigned char)ch);
    return s;
}

// ✅ PART 문자열 → enum
static DronePart parsePart(const std::string& s) {
    std::string t = toLower(s);
    if (t == "body") return DronePart::BODY;
    if (t == "arm")  return DronePart::ARM;
    if (t == "hand") return DronePart::HAND;
    return DronePart::NONE;
}

static std::vector<std::string> splitCSV(const std::string& line) {
    std::vector<std::string> out;
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, ',')) {
        size_t b = token.find_first_not_of(" \t\r\n");
        size_t e = token.find_last_not_of(" \t\r\n");
        if (b == std::string::npos) out.push_back("");
        else out.push_back(token.substr(b, e - b + 1));
    }
    return out;
}

static bool loadDronePosesCSV(const std::string& path, std::vector<DronePose>& poses, bool swapYZ)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cout << "[CSV] File open failed: " << path << "\n";
        return false;
    }

    std::string headerLine;
    if (!std::getline(f, headerLine)) {
        std::cout << "[CSV] Empty file: " << path << "\n";
        return false;
    }

    auto headers = splitCSV(headerLine);
    for (auto& h : headers) h = toLower(h);

    auto findCol = [&](const char* name) -> int {
        std::string key = toLower(name);
        for (int i = 0; i < (int)headers.size(); ++i) {
            if (headers[i] == key) return i;
        }
        return -1;
        };

    int ix = findCol("x");
    int iy = findCol("y");
    int iz = findCol("z");
    int ir = findCol("r");
    int ig = findCol("g");
    int ib = findCol("b");
    int ip = findCol("part"); // ✅ PART (optional)

    if (ix < 0 || iy < 0 || iz < 0 || ir < 0 || ig < 0 || ib < 0) {
        std::cout << "[CSV] Missing required columns. Need X,Y,Z,R,G,B\n";
        return false;
    }

    poses.clear();
    poses.reserve(1200);

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;

        auto cols = splitCSV(line);
        int needMax = std::max({ ix, iy, iz, ir, ig, ib, (ip >= 0 ? ip : 0) });
        if ((int)cols.size() <= needMax) continue;

        float X = std::stof(cols[ix]);
        float Y = std::stof(cols[iy]);
        float Z = std::stof(cols[iz]);

        float R = std::stof(cols[ir]);
        float G = std::stof(cols[ig]);
        float B = std::stof(cols[ib]);

        float maxC = std::max({ R, G, B });
        if (maxC > 1.5f) { R /= 255.0f; G /= 255.0f; B /= 255.0f; }

        Vec3 pos = swapYZ ? Vec3(X, Z, Y) : Vec3(X, Y, Z);
        Vec3 col(clamp01(R), clamp01(G), clamp01(B));

        DronePart part = DronePart::NONE;
        if (ip >= 0 && ip < (int)cols.size()) {
            part = parsePart(cols[ip]);
        }

        poses.push_back({ pos, col, part });
    }

    std::cout << "[CSV] Loaded poses: " << poses.size() << " from " << path << "\n";
    return !poses.empty();
}

bool Formation::setFromCSV(std::vector<Drone>& drones,
    const std::string& csvPath,
    float scale,
    Vec3 offset,
    bool autoCenter,
    bool swapYZ,
    bool overrideColor)
{
    if (drones.empty()) return false;

    static std::unordered_map<std::string, std::vector<DronePose>> cache;

    auto it = cache.find(csvPath);
    if (it == cache.end()) {
        std::vector<DronePose> loaded;
        if (!loadDronePosesCSV(csvPath, loaded, swapYZ)) return false;
        it = cache.emplace(csvPath, std::move(loaded)).first;
    }

    auto& poses = it->second;
    if (poses.empty()) return false;

    Vec3 center(0, 0, 0);
    if (autoCenter) {
        for (auto& p : poses) {
            center.x += p.pos.x;
            center.y += p.pos.y;
            center.z += p.pos.z;
        }
        float inv = 1.0f / (float)poses.size();
        center.x *= inv; center.y *= inv; center.z *= inv;
    }

    int usable = std::min((int)drones.size(), (int)poses.size());

    for (int i = 0; i < usable; ++i) {
        Vec3 p = poses[i].pos;

        if (autoCenter) {
            p.x -= center.x;
            p.y -= center.y;
            p.z -= center.z;
        }

        p.x = p.x * scale + offset.x;
        p.y = p.y * scale + offset.y;
        p.z = p.z * scale + offset.z;

        drones[i].target = p;

        // ✅ PART / baseTarget 저장
        drones[i].part = poses[i].part;
        drones[i].baseTarget = p;
        drones[i].hasBaseTarget = true;

        if (overrideColor) {
            drones[i].colorOverride = true;
            drones[i].overrideColor = poses[i].color;
        }
        else {
            drones[i].colorOverride = false;
        }
    }

    for (int i = usable; i < (int)drones.size(); ++i) {
        drones[i].colorOverride = false;
        drones[i].part = DronePart::NONE;
        drones[i].hasBaseTarget = false;
    }

    return true;
}

bool Formation::setFromCSV_FitStage(
    std::vector<Drone>& drones,
    const std::string& csvPath,
    Vec3 stageCenter,
    float stageHalfSize,
    bool swapYZ,
    bool overrideColor)
{
    if (drones.empty()) return false;

    static std::unordered_map<std::string, std::vector<DronePose>> cache;

    auto it = cache.find(csvPath);
    if (it == cache.end()) {
        std::vector<DronePose> loaded;
        if (!loadDronePosesCSV(csvPath, loaded, swapYZ)) return false;
        it = cache.emplace(csvPath, std::move(loaded)).first;
    }

    auto& poses = it->second;
    if (poses.empty()) return false;

    Vec3 mn = poses[0].pos;
    Vec3 mx = poses[0].pos;

    for (auto& p : poses) {
        mn.x = std::min(mn.x, p.pos.x);  mx.x = std::max(mx.x, p.pos.x);
        mn.y = std::min(mn.y, p.pos.y);  mx.y = std::max(mx.y, p.pos.y);
        mn.z = std::min(mn.z, p.pos.z);  mx.z = std::max(mx.z, p.pos.z);
    }

    Vec3 center(
        (mn.x + mx.x) * 0.5f,
        (mn.y + mx.y) * 0.5f,
        (mn.z + mx.z) * 0.5f
    );

    Vec3 extent(
        (mx.x - mn.x) * 0.5f,
        (mx.y - mn.y) * 0.5f,
        (mx.z - mn.z) * 0.5f
    );

    float maxExt = std::max({ extent.x, extent.y, extent.z });
    float scale = (maxExt > 1e-6f) ? (stageHalfSize / maxExt) : 1.0f;

    int usable = std::min((int)drones.size(), (int)poses.size());
    for (int i = 0; i < usable; ++i) {
        Vec3 p = poses[i].pos;

        p.x = (p.x - center.x) * scale + stageCenter.x;
        p.y = (p.y - center.y) * scale + stageCenter.y;
        p.z = (p.z - center.z) * scale + stageCenter.z;

        drones[i].target = p;

        // ✅ PART / baseTarget 저장
        drones[i].part = poses[i].part;
        drones[i].baseTarget = p;
        drones[i].hasBaseTarget = true;

        if (overrideColor) {
            drones[i].colorOverride = true;
            drones[i].overrideColor = poses[i].color;
        }
        else {
            drones[i].colorOverride = false;
        }
    }

    for (int i = usable; i < (int)drones.size(); ++i) {
        drones[i].colorOverride = false;
        drones[i].part = DronePart::NONE;
        drones[i].hasBaseTarget = false;
    }

    return true;
}

void Formation::setWinterDream(std::vector<Drone>& drones)
{
    setFromCSV_FitStage(drones, "winter_dream.csv", Vec3(0, 2.0f, 0), 8.0f, false, true);
}

void Formation::setIntroFromCSV(std::vector<Drone>& drones)
{
    setFromCSV_FitStage(drones, "snow.csv", Vec3(0, 2.0f, 0), 8.0f, false, true);
}

void Formation::setSanta(std::vector<Drone>& drones)
{
    setFromCSV_FitStage(drones, "santa_drones.csv", Vec3(0, 2.0f, 0), 8.0f, false, true);
}

void Formation::setElsa(std::vector<Drone>& drones)
{
    setFromCSV_FitStage(drones, "elsa_drones.csv", Vec3(0, 2.0f, 0), 8.0f, false, true);
}

void Formation::setAnna(std::vector<Drone>& drones)
{
    setFromCSV_FitStage(drones, "output.csv", Vec3(0, 2.0f, 0), 8.0f, false, true);
}

void Formation::setOlaf(std::vector<Drone>& drones)
{
    setFromCSV_FitStage(drones, "olaf_drones.csv", Vec3(0, 2.0f, 0), 8.0f, false, true);
}
