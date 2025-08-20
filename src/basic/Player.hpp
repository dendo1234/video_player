#pragma once
#include <SDL3/SDL.h>
#include <video/Video.hpp>
#include <memory>
#include <string_view>
#include "imgui.h"

class Player {
private:
    SDL_Window* window;
    float main_scale;

    bool show_demo_window = true;
    bool show_another_window = true;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGuiIO& io;

    uint64_t time{0};

public:
    Video* video;
    SDL_Renderer* renderer;

    Player(const std::string_view& name);
    ~Player();

    ImGuiIO& InitImGuiContext();
    void InitImGui();
    void GuiPass();
    uint64_t DeltaTime();

};