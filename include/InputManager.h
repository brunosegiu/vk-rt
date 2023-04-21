#pragma once

#include <functional>
#include <set>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "RefCountPtr.h"

namespace VKRT {
class Window;

class InputEventListener {
public:
    virtual void OnKeyPressed(int key){};
    virtual void OnKeyReleased(int key){};
    virtual void OnMouseMoved(glm::vec2 newPos){};
    virtual void OnLeftMouseButtonPressed(){};
    virtual void OnLeftMouseButtonReleased(){};
    virtual void OnRightMouseButtonPressed(){};
    virtual void OnRightMouseButtonReleased(){};
};

class InputManager : public RefCountPtr {
public:
    InputManager(Window* window);

    bool Update();

    void Subscribe(InputEventListener* eventListener);
    void Unsuscribe(InputEventListener* eventListener);

    enum class CursorMode { Disabled, Normal };
    void SetCursorMode(CursorMode mode);

    ~InputManager();

private:
    void KeyCallback(int key, int scancode, int action, int mods);
    void CursorPosCallback(double xpos, double ypos);
    void MouseButtonCallback(int button, int action, int mods);

    Window* mWindow;
    std::set<InputEventListener*> mEventListeners;
};
}  // namespace VKRT