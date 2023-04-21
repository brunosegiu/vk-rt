#include <chrono>

#include "Camera.h"
#include "Context.h"
#include "DebugUtils.h"
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
    VKRT_ASSERT_MSG(windowResult == Result::Success, "Couldn't create window");
    if (windowResult == Result::Success) {
        auto [contextResult, context] = window->CreateContext();
        VKRT_ASSERT_MSG(contextResult == Result::Success, "No compatible GPU found");
        if (contextResult == Result::Success) {
            ScopedRefPtr<Scene> scene = new Scene(context);
#if defined(VKRT_PLATFORM_WINDOWS)
            std::string userDir = std::getenv("USERPROFILE");
#elif defined(VKRT_PLATFORM_LINUX)
            std::string userDir = std::getenv("HOME");
#endif
            userDir = ".";
            ScopedRefPtr<Model> helmet =
                Model::Load(context, userDir + "/assets/DamagedHelmet.glb");
            ScopedRefPtr<Model> sponza = Model::Load(context, userDir + "/assets/sponza_b.gltf");
            ScopedRefPtr<Model> sphere = Model::Load(context, userDir + "/assets/sphere.gltf");
            ScopedRefPtr<Model> venus = Model::Load(context, userDir + "/assets/venus.gltf");
            std::for_each(sphere->GetMeshes().begin(), sphere->GetMeshes().end(), [](Mesh* mesh) {
                mesh->GetMaterial()->SetMetallic(1.0f);
            });
            std::for_each(venus->GetMeshes().begin(), venus->GetMeshes().end(), [](Mesh* mesh) {
                mesh->GetMaterial()->SetIndexOfRefraction(1.5f);
            });
            ScopedRefPtr<Camera> camera = new Camera(window);
            camera->SetTranslation(glm::vec3(-2.0f, -4.0f, 0.0f));
            camera->SetRotation(glm::vec3(0.0f, 180.0f, 0.0f));
            ScopedRefPtr<DirectionalLight> light = new DirectionalLight();
            light->SetIntensity(0.5f);

            ScopedRefPtr<PointLight> pointLight = new PointLight();
            pointLight->SetIntensity(30.0f);
            pointLight->SetPosition(glm::vec3(0.0f, 80.3f, -3.0f));

            ScopedRefPtr<Object> object1 = new Object(helmet);
            object1->SetTranslation(glm::vec3(-4.0f, 3.0f, 2.0f));
            object1->Rotate(glm::vec3(90.0f, 0.0f, 0.0f));
            object1->SetScale(glm::vec3(1.5f));
            ScopedRefPtr<Object> object2 = new Object(venus);
            object2->SetTranslation(glm::vec3(4.0f, 3.0f, -2.0f));
            object2->Rotate(glm::vec3(90.0f, 0.0f, 0.0f));
            object2->SetScale(glm::vec3(.5f));
            ScopedRefPtr<Object> object3 = new Object(sphere);
            object3->SetTranslation(glm::vec3(0.0f, 3.0f, 0.0f));
            object3->SetScale(glm::vec3(1.0f));
            ScopedRefPtr<Object> object4 = new Object(sponza);
            object4->Rotate(glm::vec3(90.0f, 0.0f, 0.0f));
            object4->SetScale(glm::vec3(0.03f));

            scene->AddObject(object1);
            scene->AddObject(object2);
            scene->AddObject(object3);
            scene->AddObject(object4);

            scene->AddLight(light);
            scene->AddLight(pointLight);

            scene->Commit();

            ScopedRefPtr<Renderer> renderer = new Renderer(context, scene);
            Timer timer;
            double elapsedSeconds = 0.0;
            double totalSeconds = 0.0;
            while (window->Update()) {
                light->SetDirection(glm::normalize(
                    glm::vec3(0.2f * cos(totalSeconds), -1.0f, 0.2f * sin(totalSeconds))));
                pointLight->SetPosition(glm::vec3(-35.0f, 3.0f, 5.5f * cos(totalSeconds)));
                timer.Start();
                camera->Update(elapsedSeconds);
                renderer->Render(camera);
                elapsedSeconds = timer.ElapsedSeconds();
                totalSeconds += elapsedSeconds;
            }
        }
        if (context != nullptr) {
            window->DestroyContext();
        }
    }
    return 0;
}