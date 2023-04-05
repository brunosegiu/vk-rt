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
    double ElapsedSeconds() {
        return static_cast<double>(Elapsed<std::chrono::microseconds>()) / 1000000.0;
    }

    std::chrono::steady_clock::time_point beginTime;
};

int main() {
    using namespace VKRT;
    auto [windowResult, window] = Window::Create();
    if (windowResult == Result::Success) {
        auto [contextResult, context] = window->CreateContext();
        if (contextResult == Result::Success) {
            Scene* scene = new Scene(context);
            Model* helmet = Model::Load(context, "D:/Downloads/DamagedHelmet.glb");
            Model* plane =
                Model::Load(context, "C:/Users/bruno/Code/Vulkan/data/models/plane.gltf");
            Camera* camera = new Camera(window);
            camera->SetTranslation(glm::vec3(0.0f, 0.0f, 4.0f));
            camera->SetRotation(glm::vec3(0.0f, 180.0f, 0.0f));
            Light* light = new Light();
            light->SetPosition(glm::vec3(0.0f, 3.0f, -2.0f));
            light->SetIntensity(70.0f);
            Light* light2 = new Light();
            light2->SetPosition(glm::vec3(0.0f, 5.0f, -7.0f));
            light2->SetIntensity(50.0f);
            Object* object1 = new Object(helmet);
            object1->SetTranslation(glm::vec3(-2.0f, 0.0f, 0.0f));
            object1->Rotate(glm::vec3(90.0f, 0.0f, 0.0f));
            Object* object2 = new Object(helmet);
            object2->SetTranslation(glm::vec3(2.0f, 0.0f, 0.0f));
            object2->Rotate(glm::vec3(90.0f, 0.0f, 0.0f));
            Object* object3 = new Object(plane);
            object3->SetTranslation(glm::vec3(0.0f, -5.0f, 0.0f));
            object3->Rotate(glm::vec3(0.0f, 0.0f, 0.0f));
            object3->Scale(glm::vec3(10.0f, 10.0f, 10.0f));

            scene->AddObject(object1);
            scene->AddObject(object2);
            scene->AddObject(object3);
            scene->AddLight(light);
            scene->AddLight(light2);

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
            object1->Release();
            object2->Release();
            object3->Release();
            light->Release();
            light2->Release();

            render->Release();
            camera->Release();
            plane->Release();
            helmet->Release();
            scene->Release();
            context->Release();
        }
    }
    window->Release();
}