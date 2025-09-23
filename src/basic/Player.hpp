#pragma once
#include <SDL3/SDL.h>
#include <video/Video.hpp>
#include <memory>
#include <string_view>
#include "imgui.h"
#include <basic/GuiHandler.hpp>
#include <basic/EventHandler.hpp>

inline auto SDL_WindowDeleter = [](SDL_Window* ptr) {
    SDL_DestroyWindow(ptr);
};

inline auto SDL_RendererDeleter = [](SDL_Renderer* ptr) {
    SDL_DestroyRenderer(ptr);
};

class Player {
private:
    int windowWidth{640};
    int windowHeight{360};

    const int barHeight{20};

public:
    EventHandler eventHandler{*this};

    std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)> window{CreateWindow()};
    std::unique_ptr<SDL_Renderer, decltype(SDL_RendererDeleter)> renderer{CreateRenderer()};
    GuiHandler guiHandler{this->window.get(), this->renderer.get()};
    Video video{"input3.mkv", *this};

    std::unique_ptr<SDL_Texture, SDL_TextureDeleter> videoTexture{CreateVideoTexture(windowWidth, windowHeight-barHeight, video.GetVideoWidth(), video.GetVideoHeight())};

private:
    float main_scale;

    bool show_demo_window{false};

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)> CreateWindow();
    std::unique_ptr<SDL_Renderer, decltype(SDL_RendererDeleter)> CreateRenderer();
    std::unique_ptr<SDL_Texture, SDL_TextureDeleter> CreateVideoTexture(int width, int height, int videoWidth, int videoHeight);

    uint64_t time{0};

public:

    Player();

    void GuiPass();
    void Render();
    uint64_t DeltaTime();
    void ResizeWindow(int width, int height);

    friend class EventHandler;
};