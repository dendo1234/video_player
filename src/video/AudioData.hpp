#pragma once
#include <video/Video.hpp>
#include <video/Deque.hpp>
#include <memory>
#include <SDL3/SDL.h>
#include <video/Deleters.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
}

class Video;

struct AudioData {
    Video* video;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> codecContext;
    std::unique_ptr<SwrContext, SwrContextDeleter> swrContext;
    Deque<AVPacket> packetQueue;
    Deque<AVFrame> frameQueue;
    AVRational time_base;
    double clock;
    SDL_AudioSpec audioSpec;
    SDL_AudioSpec outputAudioSpec;
    SDL_AudioStream* m_audioStream;
    int streamIndex = -1;
    SDL_Thread* audioDecoder;
    SDL_Thread* audioConsumer;

    AudioData() {}
    AudioData(Video* video) 
        : video{video} {
    }
};