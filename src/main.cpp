#include "Device.h"
#include "Window.h"

int main() {
    using namespace VKRT;
    Window* window = new Window();
    auto [result, device] = Device::Create(window);
    window->ProcessEvents();
    if (result == Result::Success) {
        device->Release();
    }
    window->Release();
}