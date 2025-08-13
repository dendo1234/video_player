#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include "basic/Player.hpp"

SDL_AppResult SDL_AppInit(void **appstate, [[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    Player* player = new Player("video player");
    *appstate = player;

    return SDL_APP_CONTINUE;
};

SDL_AppResult SDL_AppIterate(void *appstate) {
    Player* player = static_cast<Player*>(appstate);

    static const uint64_t FRAME_TIME = 16666667;
    uint64_t frameStart = SDL_GetTicksNS();

    uint64_t dt = player->DeltaTime();

    player->video->Update(dt);

    SDL_RenderClear(player->renderer);
    player->video->Render();
    SDL_RenderPresent(player->renderer);

    uint64_t frameTime = SDL_GetTicksNS() - frameStart;
    if (frameTime < FRAME_TIME) {
        SDL_DelayNS(FRAME_TIME - frameTime); 
    }
    return SDL_APP_CONTINUE;
};

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *appstate, SDL_Event *event) {

    switch (event->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    default:
        break;
    }

    return SDL_APP_CONTINUE;
};

void SDL_AppQuit(void *appstate, [[maybe_unused]] SDL_AppResult result) {
    delete static_cast<Player*>(appstate);

    return;
};

