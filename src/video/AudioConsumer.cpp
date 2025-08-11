#include <video/Video.hpp>

int AudioConsumerThread(void* userdata) {
    Video* video = static_cast<Video*>(userdata);
    AudioData* audioData = &video->m_audioData; 
    unsigned int diffCount = 0;
    double diffWeightedSum = 0.0;
    static constexpr double factor = 0.5;
    static constexpr double diffThreshold = 0.2; // 100 ms
    static constexpr unsigned int minimalDiffCount = 20;


    while (true) {
        AVFrame* frame = audioData->frameQueue.Peak();
        if (frame == nullptr) {
            return 0;
        }
        
        if (video->GetSyncClock() < frame->pts * av_q2d(audioData->time_base)) {
            continue;
        }
        int wantedSamples = frame->nb_samples;

        if (frame != nullptr) {
            double diff = video->GetSyncClock() - frame->pts * av_q2d(audioData->time_base);
            diffWeightedSum = diff + diffWeightedSum * factor;

            if (diffCount < minimalDiffCount) {
                diffCount++;
            } else {
                double diffAvg = diffWeightedSum / (1.0-factor);
                if (fabs(diffAvg) < diffThreshold) {
                    wantedSamples += diffAvg * frame->sample_rate;
                    int minsize = frame->nb_samples * 0.8;
                    int maxsize = frame->nb_samples * 1.2;

                    wantedSamples = av_clip(wantedSamples, minsize, maxsize);
                } else {
                    diffCount = 0;
                    diffWeightedSum = 0;
                    if (!SDL_ClearAudioStream(audioData->m_audioStream)) {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't clear audio stream: %s", SDL_GetError());
                    }
                    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Audio diff is too big, clearing audio stream. Diff: %f", fabs(diffAvg));
                }
            }
        }

        AVFrame* resultFrame = av_frame_alloc();
        if (resultFrame == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error to alloc AVFrame");
        }
        resultFrame->format = AV_SAMPLE_FMT_S16;
        resultFrame->ch_layout = audioData->codecContext->ch_layout;
        resultFrame->nb_samples = wantedSamples;
        int error = av_frame_get_buffer(resultFrame, 0);
        if (error < 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, error);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error av_frame_get_buffer: %s", buffer);
            SDL_Log("%d", wantedSamples);
        }

        error = swr_set_compensation(audioData->swrContext.get(), wantedSamples - frame->nb_samples, frame->sample_rate/8);
        if (error < 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, error);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error swr_set_compensation: %s", buffer);
        }


        int numberOfSamples = swr_convert(audioData->swrContext.get(), (uint8_t**)resultFrame->data, resultFrame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
        if (numberOfSamples < 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, numberOfSamples);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error swr convert: %s", buffer);
        }

        if (numberOfSamples != resultFrame->linesize[0]) {
            SDL_Log("%d %d", numberOfSamples, resultFrame->linesize[0]);
        }

        audioData->clock = frame->pts * av_q2d(audioData->time_base);

        if (SDL_GetAudioStreamQueued(audioData->m_audioStream) < 200) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Low amount of bytes on audio stream, queued: %d", SDL_GetAudioStreamQueued(audioData->m_audioStream));
        }

        if (!SDL_PutAudioStreamData(audioData->m_audioStream, resultFrame->data[0], numberOfSamples*4)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to put audio stream data: %s", SDL_GetError());
        }

        
        audioData->frameQueue.Pop();
        av_frame_unref(frame);
        av_frame_unref(resultFrame);
    }
};