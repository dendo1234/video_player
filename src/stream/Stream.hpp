#pragma once
#include <memory>
#include <thread>

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
    Deque<std::unique_ptr<AVPacket, AVPacketDeleter>, 2> packetQueue;
    Deque<std::unique_ptr<AVFrame, AVFrameDeleter>, 2> frameQueue;
    int streamIndex{-1};
    AVRational timeBase;

    static int DecoderThread(void* userdata);
    std::jthread decoderThread;

    virtual void CreateThreads() = 0;
public:
    Stream();
    Stream(Video* video, std::unique_ptr<AVCodecContext, AVCodecContextDeleter> context, int streamIndex, AVRational timeBase);
    virtual ~Stream() = default;
    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;
    Stream(Stream&&) = default;
    Stream& operator=(Stream&&) = default;

    inline int GetStreamIndex();
    inline AVRational GetTimeBase();

    void PushPacket(AVPacket* pkt);
    virtual void Flush();
    virtual void GuiPass();
};

int Stream::GetStreamIndex() {
    return streamIndex;
}

AVRational Stream::GetTimeBase() {
    return timeBase;
}