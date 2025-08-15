#include <video/Deque.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
}


template<HasPts T>
Deque<T>::Deque() {
    if (!mutex || !readCond || !writeCond) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error creating deque's locks or conditions: %s", SDL_GetError());
    }
}

// Deque<AVFrame>::~Deque() {
//     AVFrame* f;
//     while ((f = Get()) != nullptr) {
//         av_frame_free(&f);
//     }
// }

// Deque<AVPacket>::~Deque() {
//     AVPacket* f;
//     while ((f = Get()) != nullptr) {
//         av_packet_free(&f);
//     }
// }

template<HasPts T>
void Deque<T>::Push(T packet) {
    SDL_LockMutex(mutex.get());
    if (packet == nullptr) {
        flushed = true;
        SDL_SignalCondition(writeCond.get());
    } else {
        while (deque.size() > 200 && !flushed) {
            SDL_WaitCondition(writeCond.get(), mutex.get());
        }
    }
    deque.push_back(std::move(packet));
    SDL_SignalCondition(readCond.get());
    SDL_UnlockMutex(mutex.get());
    
}

template<HasPts T>
void Deque<T>::PushFront(T packet) {
    SDL_LockMutex(mutex.get());
    deque.push_front(std::move(packet));
    SDL_SignalCondition(readCond.get());
    SDL_UnlockMutex(mutex.get());
}

template<HasPts T>
T Deque<T>::Get() {
    SDL_LockMutex(mutex.get());
    while (deque.size() == 0) {
        SDL_WaitCondition(readCond.get(),mutex.get());
    }
    T ptr = std::move(deque.front());
    deque.pop_front();
    SDL_SignalCondition(writeCond.get());

    SDL_UnlockMutex(mutex.get());
    
    return ptr;
}

// template<HasPts T>
// T Deque<T>::Peak() {
//     SDL_LockMutex(mutex.get());
//     while (deque.size() == 0) {
//         SDL_WaitCondition(readCond.get(),mutex.get());
//     }
//     T ptr = deque.front();

//     SDL_UnlockMutex(mutex.get());
    
//     return ptr;
// }

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
T Deque<T>::GetBeforePts(int64_t pts) {
    SDL_LockMutex(mutex.get());
    if (deque.size() == 0) {
        SDL_WaitCondition(readCond.get(),mutex.get());
    }
    T oldPtr = nullptr;
    T ptr = std::move(deque.front());

    if (ptr == nullptr) {
        return nullptr;
    }

    while (ptr->pts < pts && deque.size() > 1) {
        oldPtr = std::move(ptr);
        deque.pop_front();
        ptr = std::move(deque.front());
        if (ptr == nullptr) {
            return nullptr;
        }
    } 

    deque.pop_front();
    PushFront(std::move(ptr));

    SDL_SignalCondition(writeCond.get());
    SDL_UnlockMutex(mutex.get());
    return oldPtr;
}

template <HasPts T>
T Deque<T>::BlockingGetBeforePts(int64_t pts) {
    SDL_LockMutex(mutex.get());
    if (deque.size() == 0) {
        SDL_WaitCondition(readCond.get(),mutex.get());
    }

    T ptr = std::move(deque.front());
    deque.pop_front();

    if (ptr == nullptr) {
        return nullptr;
    }

    while (ptr->pts < pts) {
        if (deque.size() == 0) {
            SDL_WaitCondition(readCond.get(),mutex.get());
        }
        ptr = std::move(deque.front());
        deque.pop_front();
        if (ptr == nullptr) {
            return nullptr;
        }
    }

    SDL_SignalCondition(writeCond.get());
    SDL_UnlockMutex(mutex.get());
    return ptr;
}

template class Deque<std::unique_ptr<AVFrame,AVFrameDeleter>>;
template class Deque<std::unique_ptr<AVPacket,AVPacketDeleter>>;