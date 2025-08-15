#include <stream/AudioStream.hpp>
#include <video/Video.hpp>
#include <format>

using namespace std;

AudioStream::AudioStream(Video* video, std::unique_ptr<AVCodecContext, AVCodecContextDeleter> context, int streamIndex, AVRational timeBase) 
    : Stream{video, std::move(context), streamIndex, timeBase} {

    audioSpec.freq = this->context->sample_rate;
    audioSpec.format = SDL_AUDIO_S16;
    audioSpec.channels = static_cast<uint8_t>(this->context->ch_layout.nb_channels);

    InitializeSDLStream();
    InitializeSwrContext();

    CreateThreads();
}

void AudioStream::InitializeSDLStream() {
    SDL_GetAudioDeviceFormat(video->GetAudioDeviceID(), &outputAudioSpec, nullptr);
    SDL_AudioStream* sdlAudioStreamRaw = SDL_CreateAudioStream(&audioSpec, &outputAudioSpec);
    if (sdlAudioStreamRaw == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create audio stream: %s", SDL_GetError());
    }

    sdlAudioStream = unique_ptr<SDL_AudioStream, SDL_AudioStreamDeleter>(sdlAudioStreamRaw);

    if (!SDL_BindAudioStream(video->GetAudioDeviceID(), sdlAudioStream.get())) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to bind audio stream: %s", SDL_GetError());
    }
}

void AudioStream::InitializeSwrContext() {
    AVChannelLayout inChannelLayout;
    av_channel_layout_default(&inChannelLayout, audioSpec.channels);
    SwrContext *swr = nullptr;
    int lastError = swr_alloc_set_opts2(&swr, &inChannelLayout, AV_SAMPLE_FMT_S16, audioSpec.freq, &context->ch_layout, context->sample_fmt, context->sample_rate, 0, nullptr);
    if (lastError != 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error alloc swr: %s", buffer);
    }
    lastError = swr_init(swr);
    if (lastError != 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error initing swr: %s", buffer);
    }

    swrContext = unique_ptr<SwrContext, SwrContextDeleter>(swr);
}

void AudioStream::CreateThreads() {
    string name = format("Audio decoder {}", streamIndex);
    string name2 = format("Audio consumer {}", streamIndex);
    decoderThread = unique_ptr<SDL_Thread, SDL_ThreadDeleter>(SDL_CreateThread(Stream::DecoderThread, name.c_str(), (void*)this));
    audioConsumer = unique_ptr<SDL_Thread, SDL_ThreadDeleter>(SDL_CreateThread(AudioStream::AudioConsumerThreadWrapper, name2.c_str(), (void*)this));
}

void AudioStream::Flush() {
    Stream::Flush();

    // Flush swrContext
    int numberOfSamples  = swr_convert(swrContext.get(),nullptr,0,nullptr,0);
    if (numberOfSamples < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, numberOfSamples);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error swr convert: %s", buffer);
    }
}

SDL_AudioSpec AudioStream::GetSourceAudioFormat() {
    SDL_AudioSpec audioFormat;
    SDL_GetAudioStreamFormat(sdlAudioStream.get(), &audioFormat, nullptr);
    return audioFormat;
}

SDL_AudioSpec AudioStream::GetOutputAudioFormat() {
    SDL_AudioSpec audioFormat;
    SDL_GetAudioStreamFormat(sdlAudioStream.get(), nullptr, &audioFormat);
    return audioFormat;
}

double AudioStream::GetSecondsRemainingOnStream() {
    SDL_AudioSpec audioSpec = GetOutputAudioFormat();
    double bytesPerSecond = audioSpec.freq * audioSpec.channels * SDL_AUDIO_BYTESIZE(audioSpec.format);

    double secondsRemaining = (SDL_GetAudioStreamQueued(sdlAudioStream.get())) / bytesPerSecond;
    return secondsRemaining;
}

double AudioStream::GetSecondsRemaining() {
    double swrDelay = swr_get_delay(swrContext.get(), 1000000) / 1000000;

    double secondsRemaining = GetSecondsRemainingOnStream() + swrDelay;
    return secondsRemaining;
}

double AudioStream::CalculateDiff(int64_t pts) {
    double frameStartTime = pts * av_q2d(GetTimeBase());
    double syncClock = video->GetSyncClock();
    double streamQueuedTime = GetSecondsRemaining();
    double diff = frameStartTime - (syncClock + streamQueuedTime);
    return diff;
}

int AudioStream::AudioConsumerThread() {
    unsigned int diffCount = 0;
    double diffWeightedSum = 0.0;
    static constexpr double factor = 0.8;
    static constexpr double diffThreshold = 0.03;
    static constexpr double noSyncThreshold = 10.9; // 100 ms
    static constexpr unsigned int minimalDiffCount = 10;
    static constexpr double desiredBufferSizeSeconds = 0.05;
    double diff = 0;


    while (!video->m_videoDone) {
        if (GetSecondsRemainingOnStream() > desiredBufferSizeSeconds) {
            SDL_Delay(20);
            continue;
        }
        
        std::unique_ptr<AVFrame, AVFrameDeleter> frame = frameQueue.Get();
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
                    if (!SDL_ClearAudioStream(sdlAudioStream.get())) {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't clear audio stream: %s", SDL_GetError());
                    }

                    // clear the swr buffer
                    int numberOfSamples  = swr_convert(swrContext.get(),nullptr,0,nullptr,0);
                    if (numberOfSamples < 0) {
                        char buffer[AV_ERROR_MAX_STRING_SIZE];
                        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, numberOfSamples);
                        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error swr convert: %s", buffer);
                    }
                    
                    // TODO: investigate when seeking is done
                    // frame = frameQueue.BlockingGetBeforePts(static_cast<int64_t>(video->GetSyncClock() / av_q2d(time_base)));
                }
            }
        }

        AVFrame* resultFrame = av_frame_alloc();
        if (resultFrame == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error to alloc AVFrame");
        }
        // TODO: Remove this magic numbers
        resultFrame->format = AV_SAMPLE_FMT_S16;
        resultFrame->ch_layout = context->ch_layout;
        resultFrame->nb_samples = frame->nb_samples*2; // enough space to avoid buffering in the swr context
        int error = av_frame_get_buffer(resultFrame, 0);
        if (error < 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, error);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error av_frame_get_buffer: %s", buffer);
            SDL_Log("%d", wantedSamples);
        }

        error = swr_set_compensation(swrContext.get(), wantedSamples - frame->nb_samples, frame->sample_rate/8);
        if (error < 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, error);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error swr_set_compensation: %s", buffer);
        }


        int numberOfSamples = swr_convert(swrContext.get(), (uint8_t**)resultFrame->data, resultFrame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
        if (numberOfSamples < 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, numberOfSamples);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error swr convert: %s", buffer);
        }
        
        if (SDL_GetAudioStreamQueued(sdlAudioStream.get()) < 200) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Low amount of bytes on audio stream, queued: %d, available: %d", SDL_GetAudioStreamQueued(sdlAudioStream.get()), SDL_GetAudioStreamAvailable(sdlAudioStream.get()));
        }

        // TODO: Remove this magic number
        if (!SDL_PutAudioStreamData(sdlAudioStream.get(), resultFrame->data[0], numberOfSamples*4)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to put audio stream data: %s", SDL_GetError());
        }

        // clock = frame->pts * av_q2d(GetTimeBase()) + GetSecondsRemaining();
        
        av_frame_unref(resultFrame);
    }
    return 0;
}

int AudioStream::AudioConsumerThreadWrapper(void* userdata) {
    AudioStream* audioStream = static_cast<AudioStream*>(userdata);
    return audioStream->AudioConsumerThread();
}