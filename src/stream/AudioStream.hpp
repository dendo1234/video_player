#pragma once
#include <stream/Stream.hpp>

class AudioStream : public Stream {
private:
    std::unique_ptr<SwrContext, SwrContextDeleter> swrContext;

    SDL_AudioSpec audioSpec;
    SDL_AudioSpec outputAudioSpec;
    std::unique_ptr<SDL_AudioStream, SDL_AudioStreamDeleter> sdlAudioStream;
    std::unique_ptr<SDL_Thread, SDL_ThreadDeleter> audioConsumer;

    double diff{0};
    uint32_t diffCount{0};

    void InitializeSwrContext();
    void InitializeSDLStream();

    void CreateThreads() override;

    // Audio consumer stuff
    SDL_AudioSpec GetSourceAudioFormat();
    SDL_AudioSpec GetOutputAudioFormat();
    double GetSecondsRemainingOnStream();
    double GetSecondsRemaining();
    double CalculateDiff(int64_t pts);

    int AudioConsumerThread();
    static int AudioConsumerThreadWrapper(void* userdata);
public:
    AudioStream(Video* video, std::unique_ptr<AVCodecContext, AVCodecContextDeleter> context, int streamIndex, AVRational timeBase);

    void Flush() override;
    void GuiPass() override;
};
