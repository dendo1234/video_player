#include <basic/Player.hpp>
#include <SDL3/SDL.h>

using namespace std;

Player::Player(const string_view& name) {
    SDL_SetHint(SDL_HINT_LOGGING, "verbose");
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_Init(SDL_INIT_AUDIO);

    window = SDL_CreateWindow(name.data(), 640, 360, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, nullptr);

    SDL_SetRenderLogicalPresentation(renderer, 640, 360, SDL_LOGICAL_PRESENTATION_LETTERBOX);


    video = new Video("sync.mp4", renderer);

    DeltaTime();

 
}

Player::~Player() {
    delete video;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

uint64_t Player::DeltaTime() {
    uint64_t dt = SDL_GetTicksNS() - time;
    time = SDL_GetTicksNS();

    return dt;
}