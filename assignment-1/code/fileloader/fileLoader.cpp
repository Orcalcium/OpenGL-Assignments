#include "fileLoader.h"
#include <sys/stat.h>

bool fileExists(const std::string& filePath) {
    struct stat buffer;
    return (stat(filePath.c_str(), &buffer) == 0);
}

std::string FileLoader::loadFile(const std::string& filePath) {
    std::string fileContent;
    std::ifstream fileStream;
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        if (!fileExists(filePath)) {
            std::cerr << "ERROR::FILE::NOT_FOUND: " << filePath << std::endl;
            return "";
        }
        std::cout << "Attempting to open file: " << filePath << std::endl;
        fileStream.open(filePath);
        std::stringstream contentStream;
        contentStream << fileStream.rdbuf();
        fileStream.close();
        fileContent = contentStream.str();
        std::cout << "File successfully read: " << filePath << std::endl;
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::FILE::NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
    }
    return fileContent;
}