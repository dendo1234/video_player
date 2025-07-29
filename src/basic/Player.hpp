#include <SDL3/SDL.h>
#include <video/Video.hpp>
#include <memory>

class Player {
private:
    SDL_Window* window;

    uint64_t time{0};

public:
    Video* video;
    SDL_Renderer* renderer;

    Player(const std::string_view& name);
    ~Player();
    uint64_t DeltaTime();

};