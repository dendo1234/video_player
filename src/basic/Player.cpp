#include <basic/Player.hpp>
#include <SDL3/SDL.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>


std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)> Player::CreateWindow() {
    SDL_Window* window = SDL_CreateWindow("video player", 640, 360, SDL_WINDOW_RESIZABLE);
    return std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)>(window);
}

std::unique_ptr<SDL_Renderer, decltype(SDL_RendererDeleter)> Player::CreateRenderer() {
    SDL_Renderer* renderer = SDL_CreateRenderer(window.get(), nullptr);
    return std::unique_ptr<SDL_Renderer, decltype(SDL_RendererDeleter)>(renderer);
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

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / guiHandler.io.Framerate, guiHandler.io.Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer.get());
}

uint64_t Player::DeltaTime() {
    uint64_t dt = SDL_GetTicksNS() - time;
    time = SDL_GetTicksNS();

    return dt;
}