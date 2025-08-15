#pragma once

#include <SDL3/SDL.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ptr) {
        avformat_close_input(&ptr);
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* ptr) {
        sws_freeContext(ptr);
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ptr) {
        avcodec_free_context(&ptr);
    }
};

struct SwrContextDeleter {
    void operator()(SwrContext* ptr) {
        swr_free(&ptr);
    }
};

struct AVPacketDeleter {
    void operator()(AVPacket* ptr) {
        av_packet_free(&ptr);
    }
};

struct AVFrameDeleter {
    void operator()(AVFrame* ptr) {
        av_frame_free(&ptr);
    }
};

struct SDL_TextureDeleter {
    void operator()(SDL_Texture* ptr) {
        SDL_DestroyTexture(ptr);
    }
};

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

struct SDL_ThreadDeleter {
    void operator()(SDL_Thread* ptr) {
        int status;
        SDL_WaitThread(ptr, &status);
    }
};