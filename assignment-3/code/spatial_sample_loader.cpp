#include "spatial_sample_loader.h"
#include <fstream>
#include <iostream>

bool SpatialSampleLoader::loadSS2File(const std::string& filename, std::vector<SpatialSamplePoint>& samples) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open spatial sample file: " << filename << std::endl;
        return false;
    }
    
    int numSamples;
    file.read(reinterpret_cast<char*>(&numSamples), sizeof(int));
    
    if (numSamples <= 0) {
        std::cerr << "Invalid number of samples: " << numSamples << std::endl;
        return false;
    }
    
    samples.clear();
    samples.reserve(numSamples);
    
    for (int i = 0; i < numSamples; i++) {
        SpatialSamplePoint sample;
        
        // Read position
        file.read(reinterpret_cast<char*>(&sample.position.x), sizeof(float));
        file.read(reinterpret_cast<char*>(&sample.position.y), sizeof(float));
        file.read(reinterpret_cast<char*>(&sample.position.z), sizeof(float));
        
        // Read rotation
        file.read(reinterpret_cast<char*>(&sample.rotation.x), sizeof(float));
        file.read(reinterpret_cast<char*>(&sample.rotation.y), sizeof(float));
        file.read(reinterpret_cast<char*>(&sample.rotation.z), sizeof(float));
        
        samples.push_back(sample);
    }
    
    file.close();
    std::cout << "Loaded " << samples.size() << " spatial samples from " << filename << std::endl;
    return true;
}
