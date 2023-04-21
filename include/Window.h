#pragma once
#include <string>
#include <vector>

#include "InputManager.h"
#include "RefCountPtr.h"
#include "Result.h"

class GLFWwindow;

namespace VKRT {

class Context;
class InputManager;

class Window : public RefCountPtr {
public:
    static ResultValue<ScopedRefPtr<Window>> Create();

    Window();

    bool Update();
    std::vector<std::string> GetRequiredVulkanExtensions();

    GLFWwindow* GetNativeHandle() { return mNativeHandle; }
    ScopedRefPtr<InputManager> GetInputManager() { return mInputManager; }

    struct Size2D {
        uint32_t width;
        uint32_t height;
    };
    Size2D GetSize();

    ResultValue<ScopedRefPtr<Context>> CreateContext();
    void DestroyContext();

    ~Window();

private:
    GLFWwindow* mNativeHandle;
    ScopedRefPtr<Context> mContext;
    ScopedRefPtr<InputManager> mInputManager;
};
}  // namespace VKRT
