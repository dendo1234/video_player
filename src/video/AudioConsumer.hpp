#pragma once
#include <video/Video.hpp>
#include <video/AudioData.hpp>

int AudioConsumerThread(void* userdata);

class AudioConsumer {
    AudioData& audioData;

public:
    AudioConsumer();
    AudioConsumer(AudioData* audioData);
    ~AudioConsumer();



    SDL_AudioSpec GetSourceAudioFormat();
    SDL_AudioSpec GetOutputAudioFormat();
    double GetSecondsRemainingOnStream();
    double GetSecondsRemaining();
    double CalculateDiff(int64_t pts);

    int Run();
};
