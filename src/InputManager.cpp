#include "InputManager.h"

#include <functional>

namespace VKRT {
InputManager::InputManager(Window* window) : mWindow(window) {
    glfwSetWindowUserPointer(mWindow->GetNativeHandle(), this);
#define BIND_GLWFW_CALLBACK(function)                                           \
    [](GLFWwindow* window, auto... args) {                                      \
        VKRT::InputManager* manager =                                           \
            static_cast<VKRT::InputManager*>(glfwGetWindowUserPointer(window)); \
        manager->function(args...);                                             \
    }
    glfwSetKeyCallback(mWindow->GetNativeHandle(), BIND_GLWFW_CALLBACK(InputManager::KeyCallback));
    glfwSetCursorPosCallback(
        mWindow->GetNativeHandle(),
        BIND_GLWFW_CALLBACK(InputManager::CursorPosCallback));
    glfwSetMouseButtonCallback(
        mWindow->GetNativeHandle(),
        BIND_GLWFW_CALLBACK(InputManager::MouseButtonCallback));

    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(mWindow->GetNativeHandle(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
}

bool InputManager::Update() {
    if (!glfwWindowShouldClose(mWindow->GetNativeHandle())) {
        glfwPollEvents();
        return true;
    }
    return false;
}

void InputManager::SetCursorMode(CursorMode mode) {
    switch (mode) {
        case CursorMode::Disabled: {
            glfwSetInputMode(mWindow->GetNativeHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } break;
        case CursorMode::Normal: {
            glfwSetInputMode(mWindow->GetNativeHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } break;
    }
}

void InputManager::Subscribe(InputEventListener* eventListener) {
    mEventListeners.emplace(eventListener);
}

void InputManager::Unsuscribe(InputEventListener* eventListener) {
    mEventListeners.erase(eventListener);
}

void InputManager::KeyCallback(int key, int scancode, int action, int mods) {
    for (InputEventListener* listener : mEventListeners) {
        if (action == GLFW_PRESS) {
            listener->OnKeyPressed(key);
        } else if (action == GLFW_RELEASE) {
            listener->OnKeyReleased(key);
        }
    }
}

void InputManager::CursorPosCallback(double xpos, double ypos) {
    auto windowSize = mWindow->GetSize();
    const glm::vec2 normalizedCoordinates = glm::vec2(
        xpos / static_cast<double>(windowSize.width),
        ypos / static_cast<double>(windowSize.height));
    for (InputEventListener* listener : mEventListeners) {
        listener->OnMouseMoved(normalizedCoordinates);
    }
}

void InputManager::MouseButtonCallback(int button, int action, int mods) {
    for (InputEventListener* listener : mEventListeners) {
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                listener->OnRightMouseButtonPressed();
            } else if (action == GLFW_RELEASE) {
                listener->OnRightMouseButtonReleased();
            }
        } else if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                listener->OnLeftMouseButtonPressed();
            } else if (action == GLFW_RELEASE) {
                listener->OnLeftMouseButtonReleased();
            }
        }
    }
}

InputManager::~InputManager() {
    mWindow->Release();
}

}  // namespace VKRT
