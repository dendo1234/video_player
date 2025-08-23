#include <video/Deque.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
}


template<HasPts T, int MaxSize>
Deque<T, MaxSize>::Deque() {
    if (!mutex || !cond) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error creating deque's locks or conditions: %s", SDL_GetError());
    }
}

template<HasPts T, int MaxSize>
void Deque<T, MaxSize>::Push(T packet) {
    SDL_LockMutex(mutex.get());
    while (deque.size() > MaxSize && !flushed) {
        SDL_WaitCondition(cond.get(), mutex.get());
    }
    deque.push_back(std::move(packet));
    SDL_SignalCondition(cond.get());
    SDL_UnlockMutex(mutex.get());
}

template<HasPts T, int MaxSize>
void Deque<T, MaxSize>::PushFront(T packet) {
    SDL_LockMutex(mutex.get());
    deque.push_front(std::move(packet));
    SDL_SignalCondition(cond.get());
    SDL_UnlockMutex(mutex.get());
}

template<HasPts T, int MaxSize>
T Deque<T, MaxSize>::Get() {
    SDL_LockMutex(mutex.get());
    while (deque.size() == 0) {
        SDL_WaitCondition(cond.get(),mutex.get());
    }
    T ptr = std::move(deque.front());
    deque.pop_front();
    SDL_SignalCondition(cond.get());

    SDL_UnlockMutex(mutex.get());
    
    return ptr;
}

template<HasPts T, int MaxSize>
void Deque<T, MaxSize>::Pop() {
    SDL_LockMutex(mutex.get());
    while (deque.size() == 0) {
        SDL_WaitCondition(cond.get(),mutex.get());
    }
    deque.pop_front();
    SDL_SignalCondition(cond.get());

    SDL_UnlockMutex(mutex.get());
    
}

template<HasPts T, int MaxSize>
void Deque<T, MaxSize>::Flush() {
    SDL_LockMutex(mutex.get());
    deque.clear();
    // Unsure if there should be a signal here. The lack of it results in deadlocks, 
    // and the presence may have a thread stuck at the Push method
    deque.push_back(nullptr);
    SDL_BroadcastCondition(cond.get());
    SDL_UnlockMutex(mutex.get());
}

template<HasPts T, int MaxSize>
void Deque<T, MaxSize>::Clear() {
    SDL_LockMutex(mutex.get());
    deque.clear();
    SDL_BroadcastCondition(cond.get());
    SDL_UnlockMutex(mutex.get());
}

template<HasPts T, int MaxSize>
size_t Deque<T, MaxSize>::Size() {
    return deque.size();
}

template <HasPts T, int MaxSize>
T Deque<T, MaxSize>::GetBeforePts(int64_t pts) {
    SDL_LockMutex(mutex.get());
    if (deque.empty()) {
        SDL_UnlockMutex(mutex.get());
        return nullptr;
    }

    T lastRemoved = nullptr;

    while (!deque.empty()) {
        T &frontRef = deque.front();
        if (frontRef == nullptr) {
            deque.pop_front();
            SDL_SignalCondition(cond.get());
            SDL_UnlockMutex(mutex.get());
            return nullptr;
        }
        if (frontRef->pts < pts) {
            lastRemoved = std::move(frontRef);
            deque.pop_front();
            continue;
        } else {
            break;
        }
    }

    SDL_SignalCondition(cond.get());
    SDL_UnlockMutex(mutex.get());
    return lastRemoved;
}

template <HasPts T, int MaxSize>
T Deque<T, MaxSize>::BlockingGetBeforePts(int64_t pts) {
    SDL_LockMutex(mutex.get());
    if (deque.size() == 0) {
        SDL_WaitCondition(cond.get(),mutex.get());
    }

    T ptr = std::move(deque.front());
    deque.pop_front();

    if (ptr == nullptr) {
        return nullptr;
    }

    while (ptr->pts < pts) {
        if (deque.size() == 0) {
            SDL_WaitCondition(cond.get(),mutex.get());
        }
        ptr = std::move(deque.front());
        deque.pop_front();
        if (ptr == nullptr) {
            return nullptr;
        }
    }

    SDL_SignalCondition(cond.get());
    SDL_UnlockMutex(mutex.get());
    return ptr;
}

template class Deque<std::unique_ptr<AVFrame,AVFrameDeleter>, 2>;
template class Deque<std::unique_ptr<AVFrame,AVFrameDeleter>, 10>;
template class Deque<std::unique_ptr<AVPacket,AVPacketDeleter>, 2>;
template class Deque<std::unique_ptr<AVPacket,AVPacketDeleter>, 4>;
template class Deque<std::unique_ptr<AVPacket,AVPacketDeleter>, 20>;