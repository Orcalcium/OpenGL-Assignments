#include "shader_code_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace ShaderCodeLoader {
    std::string loadShaderCode(const std::string& filePath) {
        std::ifstream in(filePath, std::ios::in | std::ios::binary);
        if (!in) {
            std::cerr << "Failed to open shader file: " << filePath << std::endl;
            return std::string();
        }

        std::ostringstream contents;
        contents << in.rdbuf();
        in.close();
        return contents.str();
    }
}
