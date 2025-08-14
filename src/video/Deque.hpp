#pragma once
#include <concepts>
#include <deque>
#include <SDL3/SDL.h>


struct SDL_MutexDeleter {
    void operator()(SDL_Mutex* ptr) {
        SDL_DestroyMutex(ptr);
    }
};

struct SDL_ConditionDeleter {
    void operator()(SDL_Condition* ptr) {
        SDL_DestroyCondition(ptr);
    }
};

template <typename T>
concept HasPts = requires(T t) {
    t.pts;
};

template<HasPts T>
class Deque {
private:
    std::deque<T*> deque;
    std::unique_ptr<SDL_Mutex, SDL_MutexDeleter> mutex{SDL_CreateMutex()};
    std::unique_ptr<SDL_Condition, SDL_ConditionDeleter> readCond{SDL_CreateCondition()};
    std::unique_ptr<SDL_Condition, SDL_ConditionDeleter> writeCond{SDL_CreateCondition()};
    bool flushed{false};
public:
    Deque();

    void Push(T* data);
    void PushFront(T* data);
    T* Get();
    // Does not delete the element
    T* Peak();
    void Pop();
    size_t Size();
    T* GetBeforePts(int64_t pts);
    T* BlockingGetBeforePts(int64_t pts);
};

using namespace std;

template<HasPts T>
Deque<T>::Deque() {
    if (!mutex || !readCond || !writeCond) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error creating deque's locks or conditions: %s", SDL_GetError());
    }
}

template<HasPts T>
void Deque<T>::Push(T* packet) {
    SDL_LockMutex(mutex.get());
    if (packet == nullptr) {
        flushed = true;
        SDL_SignalCondition(writeCond.get());
    } else {
        while (deque.size() > 200 && !flushed) {
            SDL_WaitCondition(writeCond.get(), mutex.get());
        }
    }
    deque.push_back(packet);
    SDL_SignalCondition(readCond.get());
    SDL_UnlockMutex(mutex.get());
    
}

template<HasPts T>
void Deque<T>::PushFront(T* packet) {
    SDL_LockMutex(mutex.get());
    deque.push_front(packet);
    SDL_SignalCondition(readCond.get());
    SDL_UnlockMutex(mutex.get());
}

template<HasPts T>
T* Deque<T>::Get() {
    SDL_LockMutex(mutex.get());
    while (deque.size() == 0) {
        SDL_WaitCondition(readCond.get(),mutex.get());
    }
    T* ptr = deque.front();
    deque.pop_front();
    SDL_SignalCondition(writeCond.get());

    SDL_UnlockMutex(mutex.get());
    
    return ptr;
}

template<HasPts T>
T* Deque<T>::Peak() {
    SDL_LockMutex(mutex.get());
    while (deque.size() == 0) {
        SDL_WaitCondition(readCond.get(),mutex.get());
    }
    T* ptr = deque.front();

    SDL_UnlockMutex(mutex.get());
    
    return ptr;
}

template<HasPts T>
void Deque<T>::Pop() {
    SDL_LockMutex(mutex.get());
    while (deque.size() == 0) {
        SDL_WaitCondition(readCond.get(),mutex.get());
    }
    deque.pop_front();
    SDL_SignalCondition(writeCond.get());

    SDL_UnlockMutex(mutex.get());
    
}


template<HasPts T>
size_t Deque<T>::Size() {
    return deque.size();
}

template <HasPts T>
T* Deque<T>::GetBeforePts(int64_t pts) {
    SDL_LockMutex(mutex.get());
    if (deque.size() == 0) {
        SDL_WaitCondition(readCond.get(),mutex.get());
    }
    T* oldPtr = nullptr;
    T* ptr = deque.front();

    if (ptr == nullptr) {
        return nullptr;
    }

    while (ptr->pts < pts && deque.size() > 1) {
        if (oldPtr != nullptr) {
            av_frame_free(&oldPtr);
        };
        oldPtr = ptr;
        deque.pop_front();
        ptr = deque.front();
        if (ptr == nullptr) {
            return nullptr;
        }
    } 

    SDL_SignalCondition(writeCond.get());
    SDL_UnlockMutex(mutex.get());
    return oldPtr;
}

template <HasPts T>
T* Deque<T>::BlockingGetBeforePts(int64_t pts) {
    SDL_LockMutex(mutex.get());
    if (deque.size() == 0) {
        SDL_WaitCondition(readCond.get(),mutex.get());
    }

    T* ptr = deque.front();
    deque.pop_front();

    if (ptr == nullptr) {
        return nullptr;
    }

    while (ptr->pts < pts) {
        if (deque.size() == 0) {
            SDL_WaitCondition(readCond.get(),mutex.get());
        }
        ptr = deque.front();
        deque.pop_front();
        if (ptr == nullptr) {
            return nullptr;
        }
    }

    SDL_SignalCondition(writeCond.get());
    SDL_UnlockMutex(mutex.get());
    return ptr;
}