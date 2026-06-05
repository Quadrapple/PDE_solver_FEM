#include "event_handler.h"
#include <iostream>
#include <GLFW/glfw3.h>

EventHandler::EventHandler(std::string title, glm::uvec2 size) : State(title, size), cl(this) {
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    prevTime = glfwGetTime();

    for(int i = 0; i < GLFW_KEY_LAST; i++) {
        keyboardState[i] = false;
    }

    //Set prevPos to current cursor position to avoid camera jumps
    glfwGetCursorPos(window, &prevCursorPos.x, &prevCursorPos.y);
    setCallbacks();
    enableMouse();
}

void EventHandler::framebufferSizeCallback(GLFWwindow *window, int width, int height) {
//  getContext().setWindowSize({width, height});
}


void EventHandler::mouseClickCallback(GLFWwindow *window, int button, int action, int mods) {
    if(mouseEnabled) {
        for(auto ptr: mouseListeners) {
            ptr->onMouseClick({button, action, mods});
        }
    }
}

void EventHandler::cursorCallback(GLFWwindow *window, double xpos, double ypos) {
    if(mouseEnabled) {
        for(auto ptr: cursorListeners) {
            ptr->onCursorMove({glm::vec2(xpos, ypos), prevCursorPos});
        }
    }
    prevCursorPos = {xpos, ypos};
}

void EventHandler::keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
        keyboardState[key] = true;
    } else if(action == GLFW_RELEASE) {
        keyboardState[key] = false;
    }

    for(auto ptr: keyPressListeners) {
        ptr->onKeyboardAction({key, scancode, action, mods});
    }
}

void EventHandler::processKeysHeldDown(GLFWwindow* window) {
    for(auto ptr: keyHoldListeners) {
        ptr->onKeyHold(keyboardState);
    }
}

void EventHandler::processFrameEnd(GLFWwindow* window) {
    for(auto ptr: frameListeners) {
        ptr->onFrameEnd();
    }
}

void EventHandler::setCallbacks() {
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetMouseButtonCallback(window, mouseClickCallback);
    glfwSetKeyCallback(window, keyboardCallback);
}

void EventHandler::setCursorMode(GLenum mode) {
    glfwSetInputMode(window, GLFW_CURSOR, mode);
    cursorMode = mode;
}

void EventHandler::disableMouse() {
    if(mouseEnabled) {
        mouseEnabled = false;
        setCursorMode(GLFW_CURSOR_NORMAL);
    }
}

void EventHandler::enableMouse() {
    if(!mouseEnabled) {
        mouseEnabled = true;
        setCursorMode(GLFW_CURSOR_DISABLED);
    }
}

void EventHandler::calcDeltaTime() {
    float time = glfwGetTime();
    deltaTime = time - prevTime;
    prevTime = time;
}

void EventHandler::pollEvents() {
    glfwPollEvents();
    calcDeltaTime();
    processKeysHeldDown(window);
    processFrameEnd(window);
}

void EventHandler::addListener(CursorListener* l) {
    cursorListeners.push_back(l);
}
void EventHandler::addListener(KeyPressListener* l) {
    keyPressListeners.push_back(l);
}
void EventHandler::addListener(KeyHoldListener* l) {
    keyHoldListeners.push_back(l);
}
void EventHandler::addListener(MouseClickListener* l) {
    mouseListeners.push_back(l);
}
void EventHandler::addListener(FrameListener* l) {
    frameListeners.push_back(l);
}

EventHandler::ControlKeyListener::ControlKeyListener(EventHandler *ctx) {
    this->ctx = ctx;
    ctx->addListener((KeyPressListener*)this);
}

void EventHandler::ControlKeyListener::onKeyboardAction(KeyPress press) {
    if(press.action != GLFW_PRESS) {
        return;
    }
    switch(press.key) {
        case GLFW_KEY_P:
            if(keyPressed) {
                ctx->enableMouse();
            } else {
                ctx->disableMouse();
            }
            keyPressed  = !keyPressed;
            break;
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(ctx->window, true);
            break;
    }
}
