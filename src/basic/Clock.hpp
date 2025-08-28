#pragma once
#include <SDL3/SDL.h>

class Clock {
    double time{0.0};
    uint64_t updated_at{SDL_GetTicksNS()}; // ns

    bool paused{false};
    // seeking is like paused, but activated during seeking and deactivated after the seek
    bool seeking{false};
public:

    Clock();
    Clock(double time);
    ~Clock();

    double GetTime();
    void UpdateDt(double dt);
    void UpdateTime(double newTime);
    void SetPaused(bool paused);
    void TogglePaused();
    void SetSeeking(bool seeking);
};