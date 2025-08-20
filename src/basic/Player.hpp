#pragma once
#include <SDL3/SDL.h>
#include <video/Video.hpp>
#include <memory>
#include <string_view>
#include "imgui.h"
#include <basic/GuiHandler.hpp>

inline constexpr auto SDL_WindowDeleter = [](SDL_Window* ptr) {
    SDL_DestroyWindow(ptr);
};

inline constexpr auto SDL_RendererDeleter = [](SDL_Renderer* ptr) {
    SDL_DestroyRenderer(ptr);
};

class Player {
public:
    std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)> window{CreateWindow()};
    std::unique_ptr<SDL_Renderer, decltype(SDL_RendererDeleter)> renderer{CreateRenderer()};
    GuiHandler guiHandler{this->window.get(), this->renderer.get()};

private:
    float main_scale;

    bool show_demo_window = true;
    bool show_another_window = true;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)> CreateWindow();
    std::unique_ptr<SDL_Renderer, decltype(SDL_RendererDeleter)> CreateRenderer();

    uint64_t time{0};

public:
    Video video{"input3.mkv", this->renderer.get()};

    Player();

    void GuiPass();
    uint64_t DeltaTime();

};