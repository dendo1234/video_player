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

void Window::MaximizeWindow(int windowWidth, int windowHeight) {
    // TODO: this calculation could be a bit better
    int top, left, right, bottom;
    SDL_GetWindowBordersSize(ptr.get(), &top, &left, &bottom, &right);

    windowWidth += left + right;
    windowHeight += top;

    SDL_DisplayID id = SDL_GetDisplayForWindow(ptr.get());
    SDL_Rect rect;
    SDL_GetDisplayUsableBounds(id, &rect);

    float windowAspect = static_cast<float>(windowWidth)/windowHeight;
    float displayAspect = static_cast<float>(rect.w)/rect.h;

    SDL_Rect pos;
    //case 1: window aspect is bigger:
    if (displayAspect > windowAspect) {
        // blackbars on the left/right
        int w = windowAspect*(rect.h-top-bottom);
        pos = SDL_Rect{
            .x = (rect.w-w)/2,
            .y = top,
            .w = w,
            .h = rect.h-top-bottom
        };
    } else {
        int h = 1/windowAspect*rect.w;
        pos = SDL_Rect{
            .x = left,
            .y = (rect.h-h)/2,
            .w = rect.w-left-right,
            .h = h
        };
    }
    SDL_SetWindowSize(ptr.get(), pos.w, pos.h);
    SDL_SetWindowPosition(ptr.get(), pos.x, pos.y);
}

void Window::SwapBuffers() {
    SDL_RenderPresent(renderer.get());
}

void Window::Clear() {
    SDL_SetRenderDrawColor(renderer.get(), 20, 0, 20, 255);
    SDL_RenderClear(renderer.get());
}
