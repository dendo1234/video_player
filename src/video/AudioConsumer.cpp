#include <video/AudioConsumer.hpp>

AudioConsumer::AudioConsumer(AudioData* audioData) 
    : audioData(*audioData) {

}

AudioConsumer::~AudioConsumer() {

}

SDL_AudioSpec AudioConsumer::GetSourceAudioFormat() {
    SDL_AudioSpec audioFormat;
    SDL_GetAudioStreamFormat(audioData.m_audioStream, &audioFormat, nullptr);
    return audioFormat;
}

SDL_AudioSpec AudioConsumer::GetOutputAudioFormat() {
    SDL_AudioSpec audioFormat;
    SDL_GetAudioStreamFormat(audioData.m_audioStream, nullptr, &audioFormat);
    return audioFormat;
}

double AudioConsumer::GetSecondsRemainingOnStream() {
    SDL_AudioSpec audioSpec = GetOutputAudioFormat();
    double bytesPerSecond = audioSpec.freq * audioSpec.channels * SDL_AUDIO_BYTESIZE(audioSpec.format);

    double secondsRemaining = (SDL_GetAudioStreamQueued(audioData.m_audioStream)) / bytesPerSecond;
    return secondsRemaining;
}

double AudioConsumer::GetSecondsRemaining() {
    double swrDelay = swr_get_delay(audioData.swrContext.get(), 1000000) / 1000000;

    double secondsRemaining = GetSecondsRemainingOnStream() + swrDelay;
    return secondsRemaining;
}

double AudioConsumer::CalculateDiff([[maybe_unused]] int64_t pts) {
    double frameStartTime = pts * av_q2d(audioData.time_base);
    double syncClock = audioData.video->GetSyncClock();
    double streamQueuedTime = GetSecondsRemaining();
    double diff = frameStartTime - (syncClock + streamQueuedTime);
    return diff;
}

int AudioConsumer::Run() {
    unsigned int diffCount = 0;
    double diffWeightedSum = 0.0;
    static constexpr double factor = 0.8;
    static constexpr double diffThreshold = 0.03;
    static constexpr double noSyncThreshold = 0.5; // 100 ms
    static constexpr unsigned int minimalDiffCount = 10;
    static constexpr double desiredBufferSizeSeconds = 0.05;
    double diff = 0;


    while (!audioData.video->m_videoDone) {
        if (GetSecondsRemainingOnStream() > desiredBufferSizeSeconds) {
            SDL_Delay(20);
            continue;
        }
        
        AVFrame* frame = audioData.frameQueue.Peak();
        if (frame == nullptr) {
            return 0;
        }
        
        int wantedSamples = frame->nb_samples;

        if (frame != nullptr) {
            diff = CalculateDiff(frame->pts);
            diffWeightedSum = diff + diffWeightedSum * factor;

            if (diffCount < minimalDiffCount) {
                diffCount++;
            } else {
                double diffAvg = diffWeightedSum * (1.0-factor);
                if (fabs(diffAvg) < noSyncThreshold) {
                    if (fabs(diffAvg) > diffThreshold) {
                        wantedSamples += static_cast<int>(diffAvg * frame->sample_rate);
                        int minsize = static_cast<int>(frame->nb_samples * 0.6);
                        int maxsize = static_cast<int>(frame->nb_samples * 1.4);

                        wantedSamples = av_clip(wantedSamples, minsize, maxsize);
                    }
                } else {
                    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Audio diff is too big, clearing audio stream. Diff: %f", diffAvg);

                    diffCount = 0;
                    diffWeightedSum = 0;
                    if (!SDL_ClearAudioStream(audioData.m_audioStream)) {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't clear audio stream: %s", SDL_GetError());
                    }

                    // clear the swr buffer
                    int numberOfSamples  = swr_convert(audioData.swrContext.get(),nullptr,0,nullptr,0);
                    if (numberOfSamples < 0) {
                        char buffer[AV_ERROR_MAX_STRING_SIZE];
                        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, numberOfSamples);
                        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error swr convert: %s", buffer);
                    }
                    
                    // TODO: investigate when seeking is done
                    // frame = audioData.frameQueue.BlockingGetBeforePts(static_cast<int64_t>(video->GetSyncClock() / av_q2d(audioData.time_base)));
                }
            }
        }

        AVFrame* resultFrame = av_frame_alloc();
        if (resultFrame == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error to alloc AVFrame");
        }
        // TODO: Remove this magic numbers
        resultFrame->format = AV_SAMPLE_FMT_S16;
        resultFrame->ch_layout = audioData.codecContext->ch_layout;
        resultFrame->nb_samples = frame->nb_samples*2; // enough space to avoid buffering in the swr context
        int error = av_frame_get_buffer(resultFrame, 0);
        if (error < 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, error);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error av_frame_get_buffer: %s", buffer);
            SDL_Log("%d", wantedSamples);
        }

        error = swr_set_compensation(audioData.swrContext.get(), wantedSamples - frame->nb_samples, frame->sample_rate/8);
        if (error < 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, error);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error swr_set_compensation: %s", buffer);
        }


        int numberOfSamples = swr_convert(audioData.swrContext.get(), (uint8_t**)resultFrame->data, resultFrame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
        if (numberOfSamples < 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, numberOfSamples);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error swr convert: %s", buffer);
        }
        
        if (SDL_GetAudioStreamQueued(audioData.m_audioStream) < 200) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Low amount of bytes on audio stream, queued: %d, available: %d", SDL_GetAudioStreamQueued(audioData.m_audioStream), SDL_GetAudioStreamAvailable(audioData.m_audioStream));
        }

        // TODO: Remove this magic number
        if (!SDL_PutAudioStreamData(audioData.m_audioStream, resultFrame->data[0], numberOfSamples*4)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to put audio stream data: %s", SDL_GetError());
        }

        audioData.clock = frame->pts * av_q2d(audioData.time_base) + GetSecondsRemaining();
        
        audioData.frameQueue.Pop();
        av_frame_unref(frame);
        av_frame_unref(resultFrame);
    }
    return 0;
};

int AudioConsumerThread(void* userdata) {
    AudioConsumer audioConsumer(static_cast<AudioData*>(userdata));
    return audioConsumer.Run();
};