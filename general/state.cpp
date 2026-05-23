#include "state.h"

#include <stdexcept>
#include <iostream>

State::State(std::string title, glm::uvec2 size) {
    initGLFW();
    createWindow(title, size);
    initGLAD();

    viewportSize = size;
    hack[0] = this;
    for(int i = 0; i < 128; i++) texUnitBindings[i] = 0;

    glfwSwapInterval(0);
    enable(GL_DEPTH_TEST);
    setDepthFunc(GL_LEQUAL);
}

State& State::getContext() {
    return *hack[0];
}

float State::getDeltaTime() {
    return deltaTime;
}

void State::initGLFW() {
    if(!glfwInit()) {
        throw std::runtime_error("GLFW init failed");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, MAJOR_VER);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, MINOR_VER);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void State::initGLAD() {
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("GLAD init failed");
    }
}

void State::createWindow(std::string title, glm::uvec2 size) {
    window = glfwCreateWindow(size.x, size.y, "Asteroids", NULL, NULL);
    glfwMakeContextCurrent(window);

    if(!window) {
        throw std::runtime_error("Window creation failed");
    }
}

void State::bindBuffer(GLenum target, unsigned int id) {
    if(targets[target] != id) {
        glBindBuffer(target, id);
        targets[target] = id;
    }
}

void State::bindBufferBase(GLenum target, unsigned int index, unsigned int id) {
    BindingPoint bp{target, index};
    if(bindingPoints[bp] != id) {
        glBindBufferBase(target, index, id);
        bindingPoints[bp] = id;
    }
}

void State::bindVertexArray(unsigned int id) {
    if(boundVaoID != id) {
        glBindVertexArray(id);
        boundVaoID = id;
    }
}

void State::bindTexture(GLenum target, unsigned int unit, unsigned int id) {
    if(texUnitBindings[unit] != id) {
        activateTexUnit(unit);
        glBindTexture(target, id);
        texUnitBindings[unit] = id;
    }
}

void State::activateTexUnit(unsigned int unit) {
    if(activeTexUnit != unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        activeTexUnit = unit;
    }
}

void State::bindFramebuffer(unsigned int id) {
    if(boundFramebufID != id) {
        glBindFramebuffer(GL_FRAMEBUFFER, id);
        boundFramebufID = id;
    }
}

void State::enable(GLenum option) {
    if(enabled[option] == false) {
        glEnable(option);
        enabled[option] = true;
    }
}

void State::disable(GLenum option) {
    if(enabled[option] == true) {
        glDisable(option);
        enabled[option] = false;
    }
}

void State::useProgram(unsigned int id) {
    if(usedShaderID != id) {
        glUseProgram(id);
        usedShaderID = id;
    }
}

void State::setViewport(glm::uvec2 size) {
    glViewport(0, 0, size.x, size.y);
    this->viewportSize = size;
}

void State::setWindowSize(glm::uvec2 size) {
    setViewport(size);
    this->windowSize = size;
}

void State::viewportToWindowSize() {
    setViewport(this->windowSize);
}

glm::uvec2 State::getViewport() {
    return viewportSize;
}

glm::uvec2 State::getWindowSize() {
    return windowSize;
}

void State::setDepthFunc(GLenum option) {
    if(depthFunc != option) {
        depthFunc = option;
        glDepthFunc(option);
    }
}
