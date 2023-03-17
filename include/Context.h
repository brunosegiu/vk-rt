#pragma once

#include <string>
#include <vector>

#include "Device.h"
#include "Instance.h"
#include "RefCountPtr.h"
#include "Result.h"
#include "Window.h"

namespace VKRT {
class Context : public RefCountPtr {
public:
    static ResultValue<Context*> Create(Window* window);

    Context(Window* window, Instance* instance, Device* device);

    ~Context();

private:
    Window* mWindow;
    Instance* mInstance;
    Device* mDevice;
};
}  // namespace VKRT
