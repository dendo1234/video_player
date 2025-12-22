#include <core/Application.hpp>
#include <algorithm>
#include <core/Event.hpp>
#include <ranges>
#include <cassert>


static Application* application_ptr = nullptr;

Application::Application(const ApplicationSpecs& specs) {
    SDL_SetHint(SDL_HINT_LOGGING, "info");
    // SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_Init(SDL_INIT_AUDIO);

    
    windows.emplace_back(std::make_shared<Window>(specs.windowSpecs));

    application_ptr = this;
}

Application::~Application() {
    application_ptr = nullptr;
}

void Application::Iterate() {
    double newTime = GetTime();
    
    double dt = std::clamp(newTime - lastTime, 0.0, 0.1);
    lastTime = newTime;

    for (auto&& layer : layerStack) {
        layer->OnUpdate(dt);
    }

    for (auto&& window : windows) {
        window->SetRenderTargetSelf();
        for (auto&& layer : layerStack) {
            if (layer->windowTarget == window) {
                layer->OnRender();
            }
        }
        window->SwapBuffers();
    }
}

void Application::PushLayer(std::unique_ptr<Layer>&& layer) {
    if (!layer->windowTarget) {
        layer->windowTarget = windows[0];
    }
    layerStack.push_back(std::move(layer));
}

Application& Application::Get() {
    assert(application_ptr);
    return *application_ptr;
}


EventResult Application::RaiseEvent(Event& event) {
    for (auto&& layer : layerStack | std::views::reverse) {
        auto result = layer->OnEvent(event);
        if (result != EventResult::Continue) {
            return result;
        }
    }
    return EventResult::Continue;
}
