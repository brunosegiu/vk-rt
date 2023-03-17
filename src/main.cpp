#include "Context.h"
#include "Window.h"

int main() {
    using namespace VKRT;
    auto [windowResult, window] = Window::Create();
    if (windowResult == Result::Success) {
        auto [contextResult, context] = Context::Create(window);
        window->ProcessEvents();
        if (contextResult == Result::Success) {
            context->Release();
        }
    }
    window->Release();
}