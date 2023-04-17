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
#if defined(VKRT_PLATFORM_WINDOWS)
            std::string userDir = std::getenv("USERPROFILE");
#elif defined(VKRT_PLATFORM_LINUX)
            std::string userDir = std::getenv("HOME");
#endif
            Model* helmet = Model::Load(context, userDir + "/assets/DamagedHelmet.glb");
            /* Model* sponza =
                Model::Load(context, userDir + "/assets/sponza_b.gltf");*/
            Model* plane = Model::Load(context, userDir + "/assets/plane.gltf");
            Model* venus = Model::Load(context, userDir + "/assets/venus.gltf");
            std::for_each(venus->GetMeshes().begin(), venus->GetMeshes().end(), [](Mesh* mesh) {
                mesh->GetMaterial()->SetRoughness(1.0f);
            });
            std::for_each(plane->GetMeshes().begin(), plane->GetMeshes().end(), [](Mesh* mesh) {
                mesh->GetMaterial()->SetRoughness(0.8f);
            });
            Camera* camera = new Camera(window);
            camera->SetTranslation(glm::vec3(-2.0f, -4.0f, 0.0f));
            camera->SetRotation(glm::vec3(0.0f, 180.0f, 0.0f));
            Light* light = new Light();
            light->SetPosition(glm::vec3(-4.0f, 3.0f, 0.0f));
            light->SetIntensity(3.0f);

            Light* light1 = new Light();
            light1->SetPosition(glm::vec3(0.0f, 2.0f, 0.0f));
            light1->SetIntensity(3.0f);

            Light* light2 = new Light();
            light2->SetPosition(glm::vec3(0.0f, 1.0f, -3.0f));
            light2->SetIntensity(10.0f);

            Object* object1 = new Object(helmet);
            object1->SetTranslation(glm::vec3(-3.0f, 3.0f, 0.0f));
            object1->Rotate(glm::vec3(90.0f, 0.0f, 0.0f));
            object1->SetScale(glm::vec3(1.5f));
            Object* object2 = new Object(venus);
            object2->SetTranslation(glm::vec3(3.0f, 3.0f, 0.0f));
            object2->Rotate(glm::vec3(90.0f, 0.0f, 0.0f));
            object2->SetScale(glm::vec3(.2f));
            Object* object3 = new Object(plane);
            object3->SetTranslation(glm::vec3(0.0f, -1.0f, 0.0f));
            object3->Rotate(glm::vec3(0.0f, 0.0f, 0.0f));
            object3->SetScale(glm::vec3(10.0f));

            scene->AddObject(object1);
            scene->AddObject(object2);
            scene->AddObject(object3);

            // scene->AddLight(light);
            scene->AddLight(light1);
            scene->AddLight(light2);

            scene->Commit();

            Renderer* render = new Renderer(context, scene);
            Timer timer;
            double elapsedSeconds = 0.0;
            double totalSeconds = 0.0;
            while (window->Update()) {
                timer.Start();
                /* light2->SetPosition(glm::vec3(
                    3.0f + 2.0f * cos(totalSeconds),
                    3.0f + 2.0f * cos(totalSeconds),
                    0.0f));*/
                camera->Update(elapsedSeconds);
                render->Render(camera);
                elapsedSeconds = timer.ElapsedSeconds();
                totalSeconds += elapsedSeconds;
            }
            object1->Release();
            object2->Release();
            object3->Release();
            light->Release();
            light1->Release();
            light2->Release();
            // sponza->Release();

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