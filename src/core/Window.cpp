#include <core/Window.hpp>
#include <cassert>



Window::Window(const WindowSpecs& specs) {
    SDL_Window* window = SDL_CreateWindow(specs.title.c_str(), specs.w, specs.h, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowMinimumSize(window, 100, 30);
    ptr = std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)>(window);

    SDL_Renderer* rendererTemp = SDL_CreateRenderer(ptr.get(), nullptr);
    renderer = std::unique_ptr<SDL_Renderer, decltype(SDL_RendererDeleter)>(rendererTemp);

}

void Window::SetRenderTargetSelf() {
    SDL_SetRenderTarget(renderer.get(), nullptr);
}

std::unique_ptr<SDL_Texture, SDL_TextureDeleter> Window::CreateTexture(SDL_PixelFormat pixelFormat, SDL_TextureAccess textureAccess, int w, int h) {
    return std::unique_ptr<SDL_Texture, SDL_TextureDeleter>(SDL_CreateTexture(renderer.get(), pixelFormat, textureAccess, w, h));
}

SDL_Renderer* Window::GetRenderer() {
    assert(renderer);
    return renderer.get();
}

void Window::SwapBuffers() {
    SDL_RenderPresent(renderer.get());
}

