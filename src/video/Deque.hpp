#include <concepts>
#include <deque>
#include <SDL3/SDL.h>


template <typename T>
concept HasPts = requires(T t) {
    t.pts;
};

template<HasPts T>
class Deque {
private:
    std::deque<T*> deque;
    SDL_Mutex* mutex;
    SDL_Condition* readCond;
    SDL_Condition* writeCond;
    bool flushed{false};
public:
    Deque();
    ~Deque();

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
    mutex = SDL_CreateMutex();
    if (mutex == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error creating mutex: %s", SDL_GetError());
    }
    readCond = SDL_CreateCondition();
    if (readCond == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error creating cond: %s", SDL_GetError());
    }
    writeCond = SDL_CreateCondition();
    if (writeCond == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error creating cond: %s", SDL_GetError());
    }
}

template<HasPts T>
void Deque<T>::Push(T* packet) {
    SDL_LockMutex(mutex);
    if (packet == nullptr) {
        flushed = true;
        SDL_SignalCondition(writeCond);
    } else {
        while (deque.size() > 200 && !flushed) {
            SDL_WaitCondition(writeCond, mutex);
        }
    }
    deque.push_back(packet);
    SDL_SignalCondition(readCond);
    SDL_UnlockMutex(mutex);
    
}

template<HasPts T>
void Deque<T>::PushFront(T* packet) {
    SDL_LockMutex(mutex);
    deque.push_front(packet);
    SDL_SignalCondition(readCond);
    SDL_UnlockMutex(mutex);
}

template<HasPts T>
T* Deque<T>::Get() {
    SDL_LockMutex(mutex);
    while (deque.size() == 0) {
        SDL_WaitCondition(readCond,mutex);
    }
    T* ptr = deque.front();
    deque.pop_front();
    SDL_SignalCondition(writeCond);

    SDL_UnlockMutex(mutex);
    
    return ptr;
}

template<HasPts T>
T* Deque<T>::Peak() {
    SDL_LockMutex(mutex);
    while (deque.size() == 0) {
        SDL_WaitCondition(readCond,mutex);
    }
    T* ptr = deque.front();

    SDL_UnlockMutex(mutex);
    
    return ptr;
}

template<HasPts T>
void Deque<T>::Pop() {
    SDL_LockMutex(mutex);
    while (deque.size() == 0) {
        SDL_WaitCondition(readCond,mutex);
    }
    deque.pop_front();
    SDL_SignalCondition(writeCond);

    SDL_UnlockMutex(mutex);
    
}


template<HasPts T>
size_t Deque<T>::Size() {
    return deque.size();
}

template<HasPts T>
Deque<T>::~Deque() {
    SDL_DestroyMutex(mutex);
    SDL_DestroyCondition(readCond);
}

template <HasPts T>
T* Deque<T>::GetBeforePts(int64_t pts) {
    SDL_LockMutex(mutex);
    if (deque.size() == 0) {
        SDL_WaitCondition(readCond,mutex);
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

    SDL_SignalCondition(writeCond);
    SDL_UnlockMutex(mutex);
    return oldPtr;
}

template <HasPts T>
T* Deque<T>::BlockingGetBeforePts(int64_t pts) {
    SDL_LockMutex(mutex);
    if (deque.size() == 0) {
        SDL_WaitCondition(readCond,mutex);
    }

    T* ptr = deque.front();
    deque.pop_front();

    if (ptr == nullptr) {
        return nullptr;
    }

    while (ptr->pts < pts) {
        if (deque.size() == 0) {
            SDL_WaitCondition(readCond,mutex);
        }
        ptr = deque.front();
        deque.pop_front();
        if (ptr == nullptr) {
            return nullptr;
        }
    }

    SDL_SignalCondition(writeCond);
    SDL_UnlockMutex(mutex);
    return ptr;
}