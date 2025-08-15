#pragma once
#include <stream/Stream.hpp>
#include <video/Deleters.hpp>
#include <memory>
#include <SDL3/SDL.h>

extern "C" {
#include "libswscale/swscale.h"
}

class VideoStream : public Stream {
private:
    std::unique_ptr<SwsContext, SwsContextDeleter> swsContext;


    static int VideoDecoderThread(void* userdata);

    void InitializeSwsContext();
    void CreateThreads() override;
public:
    VideoStream(Video* video, std::unique_ptr<AVCodecContext, AVCodecContextDeleter>, int streamIndex, AVRational timeBase);


    inline int GetWidth();
    inline int GetHeight();

    void Flush() override;
    AVFrame* GetFrameBeforePts(int64_t);
};

inline int VideoStream::GetWidth() {
    return context->width;
}

inline int VideoStream::GetHeight() {
    return context->height;
}