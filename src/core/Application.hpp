#include <vector>
#include <memory>
#include <core/Layer.hpp>
#include <core/Window.hpp>
#include <core/Event.hpp>
#include <concepts>
#include <SDL3/SDL.h>

template<typename T>
concept IsLayer = std::derived_from<T, Layer>;

struct ApplicationSpecs {
    WindowSpecs windowSpecs;
};

class Application {
private:
    std::vector<std::unique_ptr<Layer>> layerStack;
    double lastTime{GetTime()};
    bool running{false};
    std::vector<std::shared_ptr<Window>> windows;

public:
    Application(const ApplicationSpecs& specs);
    ~Application();

    template<IsLayer layer, typename... Args>
    void CreateLayer(Args&&... args) {
        Layer base(windows[0]);
        auto l = std::make_unique<layer>(base, std::forward<Args>(args)...);
        layerStack.push_back(std::move(l));

    };
    void PushLayer(std::unique_ptr<Layer>&& layer);

    EventResult RaiseEvent(Event& event);
    void Iterate();
    void Stop();

    Application& Get();


    inline static double GetTime() {
        return SDL_GetTicks()/1000.0;
    }
};