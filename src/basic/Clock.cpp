#include <basic/Clock.hpp>
#include <SDL3/SDL.h>
#include <math.h>

Clock::Clock() {

}

Clock::Clock(double time)
    : time{time} {

}

Clock::~Clock() {

}

double Clock::GetTime() {
    uint64_t currentTime = SDL_GetTicksNS();
    // Race condition can make updated_at be bigger than currentTime. Because of that "underflow" could happen. int64_t avoids that
    int64_t timeDiff = !paused && !seeking ? currentTime - updated_at : 0;
    double ret = time + (timeDiff)/1e9;
    return ret;
}

// paused use in this funciton seem bad but we will see in the future
void Clock::UpdateDt(double dt) {
    if (paused || seeking) {
        return;
    }
    
    time += dt;
    updated_at = SDL_GetTicksNS();
}

void Clock::UpdateTime(double newTime) {
    time = newTime;
    updated_at = SDL_GetTicksNS();
}

void Clock::SetPaused(bool paused) {
    this->paused = paused;

    updated_at = SDL_GetTicksNS();
}

void Clock::SetSeeking(bool seeking) {
    this->seeking = seeking;

    updated_at = SDL_GetTicksNS();
}