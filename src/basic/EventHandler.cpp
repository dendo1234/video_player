#include <basic/EventHandler.hpp>
#include <basic/Player.hpp>
#include "backends/imgui_impl_sdl3.h"

EventHandler::EventHandler(Player& player)
    : player(player) {

}


SDL_AppResult EventHandler::ProcessEvent(SDL_Event* event) {
    ImGui_ImplSDL3_ProcessEvent(event);
    switch (event->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_RESIZED: {
        SDL_WindowEvent& windowEvent = event->window;
        player.ResizeWindow(windowEvent.data1, windowEvent.data2);
        break;
    }
    case SDL_EVENT_KEY_DOWN: {
        SDL_KeyboardEvent& keyEvent = event->key;
        switch (keyEvent.key) {
        case SDLK_RIGHT:
            player.video.RequestSeek(player.video.GetSyncClock() + 5, 5);
            break;
        case SDLK_LEFT:
            player.video.RequestSeek(player.video.GetSyncClock() - 5, -5);
            break;
        case SDLK_L:
            player.video.RequestSeek(player.video.GetSyncClock() + 10, 10);
            break;
        case SDLK_J:
            player.video.RequestSeek(player.video.GetSyncClock() - 10, -10);
            break;
        case SDLK_SPACE:
            player.video.TogglePause();
        default:
            break;
        }
    }
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

