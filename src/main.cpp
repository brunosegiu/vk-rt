#include <chrono>

#include "Camera.h"
#include "Context.h"
#include "Renderer.h"
#include "Scene.h"
#include "Window.h"

struct Timer {
    void Start() { beginTime = std::chrono::steady_clock::now(); }
    template <typename M>
    uint64_t Elapsed() {
        return std::chrono::duration_cast<M>(std::chrono::steady_clock::now() - beginTime).count();
    }
    uint64_t ElapsedMillis() { return Elapsed<std::chrono::milliseconds>(); }
    uint64_t ElapsedMicros() { return Elapsed<std::chrono::microseconds>(); }
    double ElapsedSeconds() { return static_cast<double>(Elapsed<std::chrono::microseconds>()) / 1000000.0; }

    std::chrono::steady_clock::time_point beginTime;
};

int main() {
    using namespace VKRT;
    auto [windowResult, window] = Window::Create();
    if (windowResult == Result::Success) {
        auto [contextResult, context] = window->CreateContext();
        if (contextResult == Result::Success) {
            Scene* scene = new Scene(context);
            Model* model = Model::Load(context, "C:/Users/bruno/Desktop/untitled.glb");
            Object* object = new Object(model, glm::mat4(1.0f));
            Camera* camera = new Camera(window);
            scene->AddObject(object);
            scene->Commit();

            Renderer* render = new Renderer(context, scene);
            Timer timer;
            double elapsedSeconds = 0.0;
            while (window->Update()) {
                timer.Start();
                camera->Update(elapsedSeconds);
                render->Render(camera);
                elapsedSeconds = timer.ElapsedSeconds();
            }
            render->Release();
            camera->Release();
            object->Release();
            model->Release();
            scene->Release();
            context->Release();
        }
    }
    window->Release();
}