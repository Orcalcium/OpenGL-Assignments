#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

struct SpatialSamplePoint {
    glm::vec3 position;
    glm::vec3 rotation;
};

class SpatialSampleLoader {
public:
    static bool loadSS2File(const std::string& filename, std::vector<SpatialSamplePoint>& samples);
};
