#pragma once

#include "InputManager.h"
#include "RefCountPtr.h"

namespace VKRT {
class Camera : public RefCountPtr, public InputEventListener {
public:
    Camera(Window* window);

    void Update(float deltaTime);

    void SetTranslation(const glm::vec3& position);
    void Translate(const glm::vec3& delta);

    void SetRotation(const glm::vec3& rotation);
    void Rotate(const glm::vec3& delta);

    glm::vec3 GetForwardDir();
    const glm::mat4& GetViewTransform() { return mViewTransform; }
    const glm::mat4& GetProjectionTransform() { return mProjectionTransform; }

    ~Camera();

private:
    void OnKeyPressed(int key) override;
    void OnKeyReleased(int key) override;
    void OnMouseMoved(glm::vec2 newPos) override;
    void OnLeftMouseButtonPressed() override;
    void OnLeftMouseButtonReleased() override;
    void OnRightMouseButtonPressed() override;
    void OnRightMouseButtonReleased() override;

    void UpdateViewTransform();

    Window* mWindow;

    glm::mat4 mViewTransform;
    glm::mat4 mProjectionTransform;

    glm::vec3 mEulerRotation;
    glm::vec3 mPosition;

    struct KeyStates {
        bool forwardPressed = false, backwardsPressed = false, leftPressed = false, rightPressed = false;
    };
    KeyStates mKeyStates;
    glm::vec2 mCurrentMousePos;
    bool mActive;
};

}  // namespace VKRT