#pragma once
#include <SDL3/SDL.h>
#include <video/Deleters.hpp>
#include <memory>
#include <string>
#include <sdl3_ttf/SDL_ttf.h>
#include <core/FontManager.hpp>


inline auto SDL_WindowDeleter = [](SDL_Window* ptr) {
    SDL_DestroyWindow(ptr);
};

inline auto SDL_RendererDeleter = [](SDL_Renderer* ptr) {
    SDL_DestroyRenderer(ptr);
};

struct TTF_TextEngineDeleter {
    void operator()(TTF_TextEngine* ptr) {
        TTF_DestroyRendererTextEngine(ptr);
        return;
    }
};

class Window {
private:
    std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)> ptr{CreateWindow()};
    std::unique_ptr<SDL_Renderer, decltype(SDL_RendererDeleter)> renderer{SDL_CreateRenderer(ptr.get(), nullptr)};
    std::unique_ptr<TTF_TextEngine, TTF_TextEngineDeleter> textEngine{TTF_CreateRendererTextEngine(renderer.get())};

    [[nodiscard]] static SDL_Window* CreateWindow();

public:
    Window();
    void Clear();
    void SwapBuffers();
    void SetRenderTargetSelf();
    void GetWindowSize(int& w, int& h);
    void GetMousePosition(float& x, float& y);

    void SetTitle(const char* title) {
        SDL_SetWindowTitle(ptr.get(), title);
    }

    void MaximizeWindow(int windowWidth, int windowHeight);

    std::unique_ptr<SDL_Texture, SDL_TextureDeleter> CreateTexture(SDL_PixelFormat pixelFormat, SDL_TextureAccess textureAccess, int w, int h);
    SDL_Renderer* GetRenderer();
    TTF_TextEngine* GetTextEngine() {
        return textEngine.get();
    }

    friend class Layer;
};
