#pragma once
#include <memory>

#include <video/Deleters.hpp>
#include <video/Deque.hpp>
#include <SDL3/SDL.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
}

class Video;

class Stream {
protected:
    Video* video;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> context;
    Deque<AVPacket> packetQueue;
    Deque<AVFrame> frameQueue;
    int streamIndex{-1};
    AVRational timeBase;

    static int DecoderThread(void* userdata);
    std::unique_ptr<SDL_Thread,SDL_ThreadDeleter> decoderThread;

    virtual void CreateThreads() = 0;
public:
    Stream();
    Stream(Video* video, std::unique_ptr<AVCodecContext, AVCodecContextDeleter> context, int streamIndex, AVRational timeBase);

    inline int GetStreamIndex();
    inline AVRational GetTimeBase();

    void PushPacket(AVPacket* pkt);
    virtual void Flush();
};

int Stream::GetStreamIndex() {
    return streamIndex;
}

AVRational Stream::GetTimeBase() {
    return timeBase;
}