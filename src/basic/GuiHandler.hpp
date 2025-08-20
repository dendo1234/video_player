#include "imgui.h"
#include <SDL3/SDL.h>

class GuiHandler {
private:
    ImGuiIO& InitImGuiContext();
    void InitImGui(SDL_Window* window, SDL_Renderer* renderer);

public:
    ImGuiIO& io{InitImGuiContext()};

    GuiHandler(SDL_Window* window, SDL_Renderer* renderer);
    ~GuiHandler();
};