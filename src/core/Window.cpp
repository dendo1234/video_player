#include <core/Window.hpp>
#include <cassert>


constexpr struct {
    std::string_view title{"masvp"};
    int w{640};
    int h{360};
} defaultWindowSpecs;

SDL_Window* Window::CreateWindow() {
    SDL_Window* window = SDL_CreateWindow(defaultWindowSpecs.title.data(), defaultWindowSpecs.w, defaultWindowSpecs.h, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowMinimumSize(window, 100, 30);
    return window;
}

Window::Window() {

}

void Window::SetRenderTargetSelf() {
    SDL_SetRenderTarget(renderer.get(), nullptr);
}

std::unique_ptr<SDL_Texture, SDL_TextureDeleter> Window::CreateTexture(SDL_PixelFormat pixelFormat, SDL_TextureAccess textureAccess, int w, int h) {
    return std::unique_ptr<SDL_Texture, SDL_TextureDeleter>(SDL_CreateTexture(renderer.get(), pixelFormat, textureAccess, w, h));
}

void Window::GetWindowSize(int& w, int& h) {
    SDL_GetWindowSize(ptr.get(), &w, &h);
}

void Window::GetMousePosition(float& x, float& y) {
    SDL_GetMouseState(&x, &y);
}

SDL_Renderer* Window::GetRenderer() {
    assert(renderer);
    return renderer.get();
}

void Window::SwapBuffers() {
    SDL_RenderPresent(renderer.get());
}

void Window::Clear() {
    SDL_SetRenderDrawColor(renderer.get(), 20, 0, 20, 255);
    SDL_RenderClear(renderer.get());
}
