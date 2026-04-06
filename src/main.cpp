#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include "core/Application.hpp"
#include "core/Event.hpp"
#include "video/Video.hpp"
#include "ui/VideoUI.hpp"
#include <cassert>

#include "ui/ClayInit.hpp"

char* clayMemory;

SDL_AppResult SDL_AppInit(void **appstate, [[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    ApplicationSpecs specs = {
        .windowSpecs = {
            .title = "Video Player",
            .w = 640,
            .h = 360,
        }
    };
    Application* app = new Application(specs);
    *appstate = app;

    clayMemory = ClayInit();

    Video* video = app->CreateLayer<Video>("F:/Users/dendo/Videos/24-10-19 01-52-04 r.mkv");
    app->CreateLayer<VideoUI>(video);


    return SDL_APP_CONTINUE;
};

SDL_AppResult SDL_AppIterate(void *appstate) {
    Application* app = static_cast<Application*>(appstate);

    static const uint64_t FRAME_TIME = 16666667;
    uint64_t frameStart = SDL_GetTicksNS();

    app->Iterate();

    uint64_t frameTime = SDL_GetTicksNS() - frameStart;
    if (frameTime < FRAME_TIME) {
        uint64_t waitTime = FRAME_TIME - frameTime;
        SDL_DelayNS(waitTime); 
    }
    return SDL_APP_CONTINUE;
};

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    Event e(event);
    Application* app = static_cast<Application*>(appstate);
    EventResult result = app->RaiseEvent(e);

    switch (result) {
        case Continue:
        case Break:
            return SDL_APP_CONTINUE;
        case TerminateSucess:
            return SDL_APP_SUCCESS;
        case TerminateError:
            return SDL_APP_FAILURE;
        default:
            assert(false);
            return SDL_APP_FAILURE;
    }
};

void SDL_AppQuit(void *appstate, [[maybe_unused]] SDL_AppResult result) {
    delete static_cast<Application*>(appstate);
    ClayDestructor(clayMemory);

    return;
};

