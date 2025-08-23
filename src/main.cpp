#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include "basic/Player.hpp"
#include "backends/imgui_impl_sdl3.h"

SDL_AppResult SDL_AppInit(void **appstate, [[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    SDL_SetHint(SDL_HINT_LOGGING, "info");
    // SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_Init(SDL_INIT_AUDIO);

    Player* player = new Player();
    *appstate = player;

    return SDL_APP_CONTINUE;
};

SDL_AppResult SDL_AppIterate(void *appstate) {
    Player* player = static_cast<Player*>(appstate);

    static const uint64_t FRAME_TIME = 16666667;
    uint64_t frameStart = SDL_GetTicksNS();

    uint64_t dt = player->DeltaTime();

    player->video.Update(dt);

    SDL_RenderClear(player->renderer.get());
    player->video.Render();
    player->GuiPass();
    SDL_RenderPresent(player->renderer.get());

    uint64_t frameTime = SDL_GetTicksNS() - frameStart;
    if (frameTime < FRAME_TIME) {
        uint64_t waitTime = FRAME_TIME - frameTime;
        SDL_DelayNS(waitTime); 
    }
    return SDL_APP_CONTINUE;
};

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    Player* player = static_cast<Player*>(appstate);

    ImGui_ImplSDL3_ProcessEvent(event);
    switch (event->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN: {
        SDL_KeyboardEvent& keyEvent = event->key;
        switch (keyEvent.key) {
        case SDLK_RIGHT:
            player->video.RequestSeek(player->video.GetSyncClock() + 5);
            break;
        case SDLK_LEFT:
            player->video.RequestSeek(player->video.GetSyncClock() - 5);
            break;
        case SDLK_L:
            player->video.RequestSeek(player->video.GetSyncClock() + 10);
            break;
        case SDLK_J:
            player->video.RequestSeek(player->video.GetSyncClock() - 10);
            break;
        default:
            break;
        }
    }
    default:
        break;
    }

    return SDL_APP_CONTINUE;
};

void SDL_AppQuit(void *appstate, [[maybe_unused]] SDL_AppResult result) {
    delete static_cast<Player*>(appstate);

    return;
};

