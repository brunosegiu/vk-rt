#pragma once
#include <string>
#include <vector>

#include "RefCountPtr.h"
#include "Result.h"

class GLFWwindow;

namespace VKRT {
class Window : public RefCountPtr {
public:
    static ResultValue<Window*> Create();

    Window();

    void ProcessEvents();
    std::vector<std::string> GetRequiredVulkanExtensions();

    void* GetWindowOSHandle();

    struct Size2D {
        uint32_t width;
        uint32_t height;
    };
    Size2D GetSize();

    ~Window();

private:
    GLFWwindow* mNativeHandle;
};
}  // namespace VKRT
