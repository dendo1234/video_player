#pragma once
#include <concepts>
#include <deque>
#include <memory>
#include <SDL3/SDL.h>
#include <video/Deleters.hpp>


template <typename T>
concept HasPts = requires(T t) {
    t->pts;
};

template<HasPts T>
class Deque {
private:
    std::deque<T> deque;
    std::unique_ptr<SDL_Mutex, SDL_MutexDeleter> mutex{SDL_CreateMutex()};
    std::unique_ptr<SDL_Condition, SDL_ConditionDeleter> cond{SDL_CreateCondition()};
    bool flushed{false};
public:
    Deque();

    void Push(T data);
    void PushFront(T data);
    T Get();
    void Pop();
    void Flush();
    size_t Size();
    T GetBeforePts(int64_t pts);
    T BlockingGetBeforePts(int64_t pts);
};

