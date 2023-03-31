#pragma once
#include <string>
#include <vector>

#include "RefCountPtr.h"
#include "Result.h"

class GLFWwindow;

namespace VKRT {

class Context;
class InputManager;

class Window : public RefCountPtr {
public:
    static ResultValue<Window*> Create();

    Window();

    bool Update();
    std::vector<std::string> GetRequiredVulkanExtensions();

    void* GetWindowOSHandle();
    GLFWwindow* GetNativeHandle() { return mNativeHandle; }
    InputManager* GetInputManager() { return mInputManager; }

    struct Size2D {
        uint32_t width;
        uint32_t height;
    };
    Size2D GetSize();

    ResultValue<Context*> CreateContext();

    ~Window();

private:
    GLFWwindow* mNativeHandle;
    Context* mContext;
    InputManager* mInputManager;
};
}  // namespace VKRT
