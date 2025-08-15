#include <stream/AudioStream.hpp>
#include <video/Video.hpp>

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
    sdlAudioStream = SDL_CreateAudioStream(&audioSpec, &outputAudioSpec);
    if (sdlAudioStream == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create audio stream: %s", SDL_GetError());
    }

    if (!SDL_BindAudioStream(video->GetAudioDeviceID(), sdlAudioStream)) {
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
    string name = "Audio decoder " + to_string(streamIndex);
    string name2 = "Audio Consumer " + to_string(streamIndex);
    decoderThread = unique_ptr<SDL_Thread, SDL_ThreadDeleter>(SDL_CreateThread(Stream::DecoderThread, name.c_str(), (void*)this));
    audioConsumer = unique_ptr<SDL_Thread, SDL_ThreadDeleter>(SDL_CreateThread(AudioConsumer::AudioConsumerThread, name2.c_str(), (void*)this));
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
