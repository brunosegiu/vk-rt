#include "Context.h"
#include "Renderer.h"
#include "Scene.h"
#include "Window.h"

int main() {
    using namespace VKRT;
    auto [windowResult, window] = Window::Create();
    if (windowResult == Result::Success) {
        auto [contextResult, context] = Context::Create(window);
        if (contextResult == Result::Success) {
            Scene* scene = new Scene(context);
            Model* model = Model::Load(context, "C:/Users/bruno/Desktop/untitled.glb");
            Object* object = new Object(model, glm::mat4(1.0f));
            scene->AddObject(object);
            scene->Commit();

            Renderer* render = new Renderer(context, scene);
            while (window->ProcessEvents()) {
                render->Render();
            }
            render->Release();
            object->Release();
            model->Release();
            scene->Release();
            context->Release();
        }
    }
    window->Release();
}