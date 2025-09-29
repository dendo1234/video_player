#include <stream/VideoStream.hpp>

using namespace std;

VideoStream::VideoStream(Video* video, std::unique_ptr<AVCodecContext, AVCodecContextDeleter> context, int streamIndex, AVRational timeBase) 
    : Stream{video, std::move(context), streamIndex, timeBase} {
    InitializeSwsContext();

    CreateThreads();
}

void VideoStream::InitializeSwsContext() {
    swsContext = unique_ptr<SwsContext, SwsContextDeleter>(sws_getContext(context->width, context->height, context->pix_fmt,
                                    context->width, context->height, AV_PIX_FMT_YUV420P12,
                                    SWS_BICUBIC, nullptr, nullptr, nullptr));

    if (!swsContext) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error initializing SwsContext");
    }
}

void VideoStream::CreateThreads() {
    decoderThread = std::jthread(Stream::DecoderThread, (void*)this);
    decoderThread.detach();
}

void VideoStream::Flush() {
    Stream::Flush();
    // Should Flush the swsContext here as well, but I didn't botther loking how to do that as it is not being used

}

std::unique_ptr<AVFrame,AVFrameDeleter> VideoStream::GetFrameBeforePts(int64_t pts) {
    return frameQueue.GetBeforePts(pts);
}