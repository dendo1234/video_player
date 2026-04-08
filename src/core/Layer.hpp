#pragma once
#include <core/Window.hpp>
#include <core/Event.hpp>
#include <memory>


class Layer {

private:
    /* data */
public:
    std::shared_ptr<Window> windowTarget;

    Layer(/* args */) {};
    Layer(std::shared_ptr<Window> windowTarget) : windowTarget{windowTarget} {}
    virtual ~Layer() = default;

    virtual void OnUpdate(double dt) { auto _ = dt; };
    virtual void OnRender() {};
    virtual EventResult OnEvent(const Event& event) {
        auto _ = event;
        return EventResult::Continue;
    };

};