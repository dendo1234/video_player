#pragma once
#include <SDL3/SDL.h>
#include <video/Deleters.hpp>
#include <memory>


inline auto SDL_WindowDeleter = [](SDL_Window* ptr) {
    SDL_DestroyWindow(ptr);
};

inline auto SDL_RendererDeleter = [](SDL_Renderer* ptr) {
    SDL_DestroyRenderer(ptr);
};

struct WindowSpecs {
    std::string title;
    int w, h;
};


class Window {
private:
    std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)> ptr;
    std::unique_ptr<SDL_Renderer, decltype(SDL_RendererDeleter)> renderer;

public:
    Window(const WindowSpecs& specs);
    void SwapBuffers();
    void SetRenderTargetSelf();
    std::unique_ptr<SDL_Texture, SDL_TextureDeleter> CreateTexture(SDL_PixelFormat pixelFormat, SDL_TextureAccess textureAccess, int w, int h);
    SDL_Renderer* GetRenderer();

    friend class Layer;
};
