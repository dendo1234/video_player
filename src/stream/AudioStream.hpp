#pragma once
#include <stream/Stream.hpp>
#include <video/AudioConsumer.hpp>

class AudioStream : public Stream {
private:
    std::unique_ptr<SwrContext, SwrContextDeleter> swrContext;

    SDL_AudioSpec audioSpec;
    SDL_AudioSpec outputAudioSpec;
    SDL_AudioStream* sdlAudioStream;
    SDL_Thread* audioDecoder;
    SDL_Thread* audioConsumer;

    void InitializeSwrContext();
    void InitializeSDLStream();

    void CreateThreads() override;

    friend class AudioConsumer;
public:
    AudioStream(Video* video, std::unique_ptr<AVCodecContext, AVCodecContextDeleter> context, int streamIndex, AVRational timeBase);

    void Flush() override;
};

