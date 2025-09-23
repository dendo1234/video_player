#include <basic/Player.hpp>
#include <SDL3/SDL.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>


std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)> Player::CreateWindow() {
    SDL_Window* window = SDL_CreateWindow("video player", windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowMinimumSize(window, 100, 30);
    return std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)>(window);
}

std::unique_ptr<SDL_Renderer, decltype(SDL_RendererDeleter)> Player::CreateRenderer() {
    SDL_Renderer* renderer = SDL_CreateRenderer(window.get(), nullptr);
    return std::unique_ptr<SDL_Renderer, decltype(SDL_RendererDeleter)>(renderer);
}

std::unique_ptr<SDL_Texture, SDL_TextureDeleter> Player::CreateVideoTexture(int width, int height) {
    SDL_Texture* texture = SDL_CreateTexture(renderer.get(), SDL_PixelFormat::SDL_PIXELFORMAT_YV12, SDL_TextureAccess::SDL_TEXTUREACCESS_TARGET, width, height);
    return std::unique_ptr<SDL_Texture, SDL_TextureDeleter>(texture);
}

Player::Player() {
    // SDL_SetRenderLogicalPresentation(renderer, 640, 360, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());


    DeltaTime();
}


void Player::GuiPass() {
    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    video.GuiPass();

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer.get());
}

uint64_t Player::DeltaTime() {
    uint64_t dt = SDL_GetTicksNS() - time;
    time = SDL_GetTicksNS();

    return dt;
}

void Player::ResizeWindow(int width, int height) {
    windowWidth = width;
    windowHeight = height;

    videoTexture = CreateVideoTexture(width, height-30);



}