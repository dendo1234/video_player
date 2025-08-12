#pragma once
#include <video/Video.hpp>

int AudioConsumerThread(void* userdata);

class AudioConsumer {
    Video* video;
    AudioData& audioData;

public:
    AudioConsumer(Video* video);
    ~AudioConsumer();



    SDL_AudioSpec GetSourceAudioFormat();
    SDL_AudioSpec GetOutputAudioFormat();
    double GetSecondsRemainingOnStream();
    double GetSecondsRemaining();
    double CalculateDiff(int64_t pts);

    int Run();
};
