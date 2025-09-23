#pragma once
#include <SDL3/SDL.h>

class Player;

class EventHandler {
    Player& player;

public:
    EventHandler(Player& player);
    SDL_AppResult ProcessEvent(SDL_Event* event);
};