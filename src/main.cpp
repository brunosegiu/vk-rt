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
            ScopedRefPtr<Model> helmetModel =
                Model::Load(context, userDir + "/assets/DamagedHelmet.glb");
            ScopedRefPtr<Model> sponzaModel =
                Model::Load(context, userDir + "/assets/sponza_b.gltf");
            ScopedRefPtr<Model> venusModel = Model::Load(context, userDir + "/assets/venus.gltf");
            ScopedRefPtr<Model> deerModel = Model::Load(context, userDir + "/assets/deer.gltf");

            std::vector<ScopedRefPtr<Model>> cubes{
                Model::Load(context, userDir + "/assets/cube.gltf"),
                Model::Load(context, userDir + "/assets/cube.gltf"),
                Model::Load(context, userDir + "/assets/cube.gltf"),
            };

            std::vector<ScopedRefPtr<Model>> spheres{
                Model::Load(context, userDir + "/assets/sphere.gltf"),
                Model::Load(context, userDir + "/assets/sphere.gltf"),
                Model::Load(context, userDir + "/assets/sphere.gltf"),
            };

            std::for_each(
                venusModel->GetMeshes().begin(),
                venusModel->GetMeshes().end(),
                [](Mesh* mesh) {
                    mesh->GetMaterial()->SetMetallic(1.0f);
                    mesh->GetMaterial()->SetRoughness(0.0f);
                });

            std::for_each(
                deerModel->GetMeshes().begin(),
                deerModel->GetMeshes().end(),
                [](Mesh* mesh) { mesh->GetMaterial()->SetIndexOfRefraction(1.5f); });

            std::for_each(
                cubes[0]->GetMeshes().begin(),
                cubes[0]->GetMeshes().end(),
                [](Mesh* mesh) {
                    mesh->GetMaterial()->SetAlbedo(glm::vec3(0.0f, 0.0f, 1.0f));
                    mesh->GetMaterial()->SetRoughness(0.5f);
                });

            std::for_each(
                cubes[1]->GetMeshes().begin(),
                cubes[1]->GetMeshes().end(),
                [](Mesh* mesh) {
                    mesh->GetMaterial()->SetMetallic(0.8f);
                    mesh->GetMaterial()->SetRoughness(0.0f);
                });

            std::for_each(
                cubes[2]->GetMeshes().begin(),
                cubes[2]->GetMeshes().end(),
                [](Mesh* mesh) { mesh->GetMaterial()->SetIndexOfRefraction(1.3f); });

            std::for_each(
                spheres[0]->GetMeshes().begin(),
                spheres[0]->GetMeshes().end(),
                [](Mesh* mesh) {
                    mesh->GetMaterial()->SetAlbedo(glm::vec3(0.2f, 0.6f, 0.3f));
                    mesh->GetMaterial()->SetRoughness(0.8f);
                });

            std::for_each(
                spheres[1]->GetMeshes().begin(),
                spheres[1]->GetMeshes().end(),
                [](Mesh* mesh) {
                    mesh->GetMaterial()->SetMetallic(1.0f);
                    mesh->GetMaterial()->SetRoughness(0.0f);
                });

            std::for_each(
                spheres[2]->GetMeshes().begin(),
                spheres[2]->GetMeshes().end(),
                [](Mesh* mesh) { mesh->GetMaterial()->SetIndexOfRefraction(1.8f); });

            ScopedRefPtr<Camera> camera = new Camera(window);
            camera->SetTranslation(glm::vec3(-2.0f, -4.0f, 0.0f));
            camera->SetRotation(glm::vec3(0.0f, 180.0f, 0.0f));

            ScopedRefPtr<DirectionalLight> light = new DirectionalLight();
            light->SetIntensity(0.8f);

            ScopedRefPtr<PointLight> pointLight = new PointLight();
            pointLight->SetIntensity(30.0f);
            pointLight->SetPosition(glm::vec3(0.0f, 80.3f, -3.0f));

            ScopedRefPtr<Object> helmet = new Object(helmetModel);
            helmet->SetTranslation(glm::vec3(4.0f, 3.0f, 0.0f));
            helmet->Rotate(glm::vec3(90.0f, 0.0f, 0.0f));
            helmet->SetScale(glm::vec3(1.5f));

            ScopedRefPtr<Object> deer = new Object(deerModel);
            deer->SetTranslation(glm::vec3(-4.0f, 3.0f, 0.0f));
            deer->SetScale(glm::vec3(.7f));

            ScopedRefPtr<Object> venus = new Object(venusModel);
            venus->SetTranslation(glm::vec3(0.0f, 3.0f, 0.0f));
            venus->SetScale(glm::vec3(0.5f));
            venus->Rotate(glm::vec3(90.0f, 0.0f, 0.0f));

            ScopedRefPtr<Object> sponza = new Object(sponzaModel);
            sponza->Rotate(glm::vec3(90.0f, 0.0f, 0.0f));
            sponza->SetScale(glm::vec3(0.03f));

            float offset = 0.0f;
            for (const auto& cube : cubes) {
                ScopedRefPtr<Object> cubeObject = new Object(cube);
                cubeObject->SetTranslation(glm::vec3(-20.0f, 2.0f, 2.0f - offset));
                cubeObject->SetScale(glm::vec3(0.3f));
                cubeObject->Rotate(glm::vec3(0.0f, offset * 20, 0.0f));
                scene->AddObject(cubeObject);
                offset += 3.0f;
            }

            offset = 0.0f;
            for (const auto& sphere : spheres) {
                ScopedRefPtr<Object> object = new Object(sphere);
                object->SetTranslation(glm::vec3(-20.0f, 4.0f, 2.0f - offset));
                object->SetScale(glm::vec3(0.75f));
                scene->AddObject(object);
                offset += 3.0f;
            }

            scene->AddObject(helmet);
            scene->AddObject(venus);
            scene->AddObject(deer);
            scene->AddObject(sponza);

            scene->AddLight(light);
            scene->AddLight(pointLight);

            ScopedRefPtr<Renderer> renderer = new Renderer(context, scene);
            Timer timer;
            double elapsedSeconds = 0.0;
            double totalSeconds = 0.0;
            while (window->Update()) {
                timer.Start();
                {
                    light->SetDirection(
                        glm::normalize(glm::vec3(0.2f, -1.0f, 0.2 * cos(totalSeconds / 2.0f))));
                    pointLight->SetPosition(glm::vec3(-35.0f, 3.0f, 5.5f * cos(totalSeconds)));
                    helmet->Rotate(glm::vec3(0.0f, elapsedSeconds * 30.0f, 0.0f));
                    venus->Rotate(glm::vec3(0.0f, elapsedSeconds * 30.0f, 0.0f));
                    deer->Rotate(glm::vec3(0.0f, elapsedSeconds * 30.0f, 0.0f));
                    deer->SetTranslation(glm::vec3(4.0f, 3.0f, 2.0f * cos(totalSeconds)));
                    camera->Update(elapsedSeconds);
                    renderer->Render(camera);
                }
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