#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace VKRT {
Camera::Camera(Window* window)
    : mWindow(window),
      mMovementSpeed(2.0f),
      mRotationSpeed(100.0f),
      mActive(false),
      mCurrentMousePos(0.0, 0.0) {
    mWindow->AddRef();
    InputManager* inputManager = mWindow->GetInputManager();
    inputManager->Subscribe(this);

    mEulerRotation = glm::vec3(0.0);
    mPosition = glm::vec3(0.0);
    auto windowSize = mWindow->GetSize();
    mProjectionTransform = glm::perspective(
        glm::radians(60.0),
        static_cast<double>(windowSize.width) / static_cast<double>(windowSize.height),
        0.01,
        1000.0);
    mProjectionTransform[1][1] *= -1.0f;
}

void Camera::UpdateViewTransform() {
    glm::mat4 rotationTransform = glm::mat4(1.0f);
    rotationTransform =
        glm::rotate(rotationTransform, glm::radians(mEulerRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotationTransform =
        glm::rotate(rotationTransform, glm::radians(mEulerRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationTransform =
        glm::rotate(rotationTransform, glm::radians(mEulerRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 translationTransform = glm::translate(glm::mat4(1.0f), mPosition);

    mViewTransform = rotationTransform * translationTransform;
}

glm::vec3 Camera::GetForwardDir() {
    const glm::mat4 invertedView = glm::inverse(mViewTransform);
    return glm::normalize(glm::vec3(invertedView[2]));
}

void Camera::Update(float deltaTime) {
    const glm::vec3 forwardDir = GetForwardDir();
    const float moveDelta = deltaTime * mMovementSpeed;
    if (mKeyStates.forwardPressed) {
        mPosition += forwardDir * moveDelta;
    }
    if (mKeyStates.backwardsPressed) {
        mPosition += -forwardDir * moveDelta;
    }
    const glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, glm::vec3(0.0f, 1.0f, 0.0f)));
    if (mKeyStates.rightPressed) {
        mPosition += rightDir * moveDelta;
    }
    if (mKeyStates.leftPressed) {
        mPosition += -rightDir * moveDelta;
    }
    UpdateViewTransform();
}

void Camera::SetTranslation(const glm::vec3& position) {
    mPosition = position;
}

void Camera::Translate(const glm::vec3& delta) {
    mPosition += delta;
}

void Camera::SetRotation(const glm::vec3& rotation) {
    mEulerRotation = rotation;
}

void Camera::Rotate(const glm::vec3& delta) {
    mEulerRotation += delta;
}

void Camera::OnKeyPressed(int key) {
    if (key == GLFW_KEY_W) {
        mKeyStates.forwardPressed = true;
    } else if (key == GLFW_KEY_S) {
        mKeyStates.backwardsPressed = true;
    } else if (key == GLFW_KEY_D) {
        mKeyStates.rightPressed = true;
    } else if (key == GLFW_KEY_A) {
        mKeyStates.leftPressed = true;
    }
}

void Camera::OnKeyReleased(int key) {
    if (key == GLFW_KEY_W) {
        mKeyStates.forwardPressed = false;
    } else if (key == GLFW_KEY_S) {
        mKeyStates.backwardsPressed = false;
    } else if (key == GLFW_KEY_D) {
        mKeyStates.rightPressed = false;
    } else if (key == GLFW_KEY_A) {
        mKeyStates.leftPressed = false;
    }
}

void Camera::OnMouseMoved(glm::vec2 newPos) {
    if (mActive) {
        glm::vec2 delta = newPos - mCurrentMousePos;
        Rotate(glm::vec3(delta.y * mRotationSpeed, delta.x * mRotationSpeed, 0.0f));
    }
    mCurrentMousePos = newPos;
}

void Camera::OnLeftMouseButtonPressed() {
    mActive = !mActive;
    InputManager* inputManager = mWindow->GetInputManager();
    inputManager->SetCursorMode(
        mActive ? InputManager::CursorMode::Disabled : InputManager::CursorMode::Normal);
}

void Camera::OnLeftMouseButtonReleased() {}

void Camera::OnRightMouseButtonPressed() {}

void Camera::OnRightMouseButtonReleased() {}

Camera::~Camera() {
    InputManager* inputManager = mWindow->GetInputManager();
    inputManager->Unsuscribe(this);
    mWindow->Release();
}
}  // namespace VKRT
