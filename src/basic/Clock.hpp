#include <SDL3/SDL.h>

class Clock {
    double time{0.0};
    uint64_t updated_at{SDL_GetTicksNS()}; // ns

public:
    bool paused{false};

    Clock();
    Clock(double time);
    ~Clock();

    double GetTime();
    void UpdateDt(double dt);
    void UpdateTime(double newTime);
};