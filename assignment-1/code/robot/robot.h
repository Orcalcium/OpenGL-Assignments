#include "../../include/glm/glm/glm.hpp"
#include "../../include/glm/glm/gtc/matrix_transform.hpp"
class Robot {
    public:
        void draw(unsigned int shaderProgram,unsigned int cubeVAO,glm::vec3 cameraPos, glm::vec3 cameraFront, glm::vec3 cameraUp,float WINDOW_WIDTH, float WINDOW_HEIGHT) ;
        bool rotate = false;
    private:
        void drawCube(unsigned int shaderProgram, glm::mat4 model,unsigned int cubeVAO,glm::vec3 cameraPos, glm::vec3 cameraFront, glm::vec3 cameraUp,float WINDOW_WIDTH, float WINDOW_HEIGHT);

};