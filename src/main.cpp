#include "Device.h"

int main() {
    using namespace VKRT;
    auto [result, device] = Device::Create();
    if (result == Result::Success) {
        device->Release();
    }
}