#pragma once
#include <stream/AudioStream.hpp>

class AudioStream;

class AudioConsumer {
    static SDL_AudioSpec GetSourceAudioFormat(AudioStream* audioStream);
    static SDL_AudioSpec GetOutputAudioFormat(AudioStream* audioStream);
    static double GetSecondsRemainingOnStream(AudioStream* audioStream);
    static double GetSecondsRemaining(AudioStream* audioStream);
    static double CalculateDiff(AudioStream* audioStream, int64_t pts);

public:
    static int AudioConsumerThread(void* userdata);

};
