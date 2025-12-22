#pragma once
#include <SDL3/SDL.h>

enum EventResult {
    Break,
    Continue,
    TerminateSucess,
    TerminateError,
};

class Event
{
public:
    bool handled = false;
    SDL_Event* sdl_event;
    Event(SDL_Event* event) {
        sdl_event = event;
    }
};

