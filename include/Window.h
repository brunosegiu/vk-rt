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

    ~Window();
private:
    GLFWwindow* mNativeHandle;
};
}  // namespace VKRT
