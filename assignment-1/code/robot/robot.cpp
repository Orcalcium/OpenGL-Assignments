#include "robot.h"
#include "../../include/glad/glad.h"
#include "../../include/glm/glm/glm.hpp"
#include "../../include/glm/glm/gtc/matrix_transform.hpp"
#include "../../include/glm/glm/gtc/type_ptr.hpp"
#include "../../include/GLFW/install/include/GLFW/glfw3.h"
#include "../textureloader/textureloader.h" // Include the header file

void Robot::drawCube(unsigned int shaderProgram, glm::mat4 model, unsigned int cubeVAO, glm::vec3 cameraPos, glm::vec3 cameraFront, glm::vec3 cameraUp, float WINDOW_WIDTH, float WINDOW_HEIGHT) {
    glUseProgram(shaderProgram);

    // Transformation matrices
    glm::mat4 view  = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieve uniform locations
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    // Pass matrices to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Render the cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Robot::draw(unsigned int shaderProgram, unsigned int cubeVAO, glm::vec3 cameraPos, glm::vec3 cameraFront, glm::vec3 cameraUp, float WINDOW_WIDTH, float WINDOW_HEIGHT) {
    TextureLoader textureLoader;
    unsigned int Texture = textureLoader.loadTexture("./image.png");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    glm::mat4 model = glm::mat4(1.0f);

    // Draw torso
    glm::mat4 torsoPos = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    if (rotate) {
        torsoPos = glm::rotate(torsoPos, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    glm::mat4 torso = glm::scale(torsoPos, glm::vec3(1.0f, 2.0f, 0.5f));
    drawCube(shaderProgram, torso, cubeVAO, cameraPos, cameraFront, cameraUp, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Draw head
    glm::mat4 headPos = glm::translate(torsoPos, glm::vec3(0.0f, 1.25f, 0.0f));
    if (rotate) {
        headPos = glm::rotate(headPos, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around Y-axis
    }
    glm::mat4 head = glm::scale(headPos, glm::vec3(0.5f, 0.5f, 0.5f));
    drawCube(shaderProgram, head, cubeVAO, cameraPos, cameraFront, cameraUp, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Draw left arm
    glm::mat4 leftArmPos = glm::translate(torsoPos, glm::vec3(-0.6f, 1.0f, 0.0f));
    if (rotate) {
        leftArmPos = glm::rotate(leftArmPos, (float)glfwGetTime(), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis
    }
    leftArmPos = glm::translate(leftArmPos, glm::vec3(0.0f, -0.5f, 0.0f));
    glm::mat4 leftArm = glm::scale(leftArmPos, glm::vec3(0.25f, 1.0f, 0.25f));
    drawCube(shaderProgram, leftArm, cubeVAO, cameraPos, cameraFront, cameraUp, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Draw left hand
    glm::mat4 leftHandPos = glm::translate(leftArmPos, glm::vec3(0.0f, -0.5f, 0.0f));
    if (rotate) {
        leftHandPos = glm::rotate(leftHandPos, (float)glfwGetTime(), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis
    }
    leftHandPos = glm::translate(leftHandPos, glm::vec3(0.0f, -0.25f, 0.0f));
    glm::mat4 leftHand = glm::scale(leftHandPos, glm::vec3(0.5f, 0.5f, 0.5f));
    drawCube(shaderProgram, leftHand, cubeVAO, cameraPos, cameraFront, cameraUp, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Draw right arm
    glm::mat4 rightArmPos = glm::translate(torsoPos, glm::vec3(0.6f, 1.0f, 0.0f));
    if (rotate) {
        rightArmPos = glm::rotate(rightArmPos, (float)glfwGetTime(), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis
    }
    rightArmPos = glm::translate(rightArmPos, glm::vec3(0.0f, -0.5f, 0.0f));
    glm::mat4 rightArm = glm::scale(rightArmPos, glm::vec3(0.25f, 1.0f, 0.25f));
    drawCube(shaderProgram, rightArm, cubeVAO, cameraPos, cameraFront, cameraUp, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Draw right hand
    glm::mat4 rightHandPos = glm::translate(rightArmPos, glm::vec3(0.0f, -0.5f, 0.0f));
    if (rotate) {
        rightHandPos = glm::rotate(rightHandPos, (float)glfwGetTime(), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis
    }
    rightHandPos = glm::translate(rightHandPos, glm::vec3(0.0f, -0.25f, 0.0f));
    glm::mat4 rightHand = glm::scale(rightHandPos, glm::vec3(0.5f, 0.5f, 0.5f));
    drawCube(shaderProgram, rightHand, cubeVAO, cameraPos, cameraFront, cameraUp, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Draw left leg
    glm::mat4 leftLegPos = glm::translate(torsoPos, glm::vec3(-0.5f, -1.5f, 0.0f));
    if (rotate) {
        leftLegPos = glm::rotate(leftLegPos, (float)glfwGetTime(), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis
    }
    glm::mat4 leftLeg = glm::scale(leftLegPos, glm::vec3(0.5f, 1.0f, 0.5f));
    drawCube(shaderProgram, leftLeg, cubeVAO, cameraPos, cameraFront, cameraUp, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Draw left foot
    glm::mat4 leftFootPos = glm::translate(leftLegPos, glm::vec3(0.0f, -1.0f, 0.0f));
    if (rotate) {
        leftFootPos = glm::rotate(leftFootPos, (float)glfwGetTime(), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis
    }
    glm::mat4 leftFoot = glm::scale(leftFootPos, glm::vec3(0.5f, 0.25f, 0.5f));
    drawCube(shaderProgram, leftFoot, cubeVAO, cameraPos, cameraFront, cameraUp, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Draw right leg
    glm::mat4 rightLegPos = glm::translate(torsoPos, glm::vec3(0.5f, -1.5f, 0.0f));
    if (rotate) {
        rightLegPos = glm::rotate(rightLegPos, (float)glfwGetTime(), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis
    }
    glm::mat4 rightLeg = glm::scale(rightLegPos, glm::vec3(0.5f, 1.0f, 0.5f));
    drawCube(shaderProgram, rightLeg, cubeVAO, cameraPos, cameraFront, cameraUp, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Draw right foot
    glm::mat4 rightFootPos = glm::translate(rightLegPos, glm::vec3(0.0f, -1.0f, 0.0f));
    if (rotate) {
        rightFootPos = glm::rotate(rightFootPos, (float)glfwGetTime(), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis
    }
    glm::mat4 rightFoot = glm::scale(rightFootPos, glm::vec3(0.5f, 0.25f, 0.5f));
    drawCube(shaderProgram, rightFoot, cubeVAO, cameraPos, cameraFront, cameraUp, WINDOW_WIDTH, WINDOW_HEIGHT);
}