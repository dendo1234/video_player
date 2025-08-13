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
    int64_t timeDiff = currentTime - updated_at;
    double ret = time + (timeDiff)/1e9;
    SDL_assert(fabs(ret) < 1000);
    return ret;
}

void Clock::UpdateDt(double dt) {
    if (paused) {
        return;
    }
    
    time += dt;
    updated_at = SDL_GetTicksNS();
}

void Clock::UpdateTime(double newTime) {
    if (paused) {
        return;
    }

    time = newTime;
    updated_at = SDL_GetTicksNS();
}