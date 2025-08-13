#include <video/Decoders.hpp>
#include <video/Video.hpp>

int VideoDecodeThread(void* userdata) {
    int lastError = 0;
    Video* video = (Video*)userdata;

    AVFrame* decodedFrame = av_frame_alloc();
    AVPacket* packet;
    bool decoding = true;

    while (true) {
        lastError = avcodec_receive_frame(video->m_videoData.m_videoCodecContext.get(), decodedFrame);

        if (lastError == 0) {
            // recebeu frame
            video->m_videoData.frameQueue.Push(decodedFrame);
            decodedFrame = av_frame_alloc();
            continue;
        }

        else if (lastError == AVERROR_EOF || video->m_videoDone) {
            // acabou os frames
            break;
        }

        if (lastError != AVERROR(EAGAIN)) {
            // erro qualquer
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error reciving video frame: %s", buffer);
            continue;
        }

        if (decoding) {
            // precisa de mais packets
            packet = video->m_videoData.packetQueue.Get();

            if (packet == nullptr) {
                decoding = false;
            }

            lastError = avcodec_send_packet(video->m_videoData.m_videoCodecContext.get(), packet);
            if (lastError == 0) {
                // packet recebida com sucesso
                if (packet != nullptr) {
                    av_packet_unref(packet);
                }
                continue;
            }

            else if (lastError == AVERROR(EAGAIN)) {
                // precisa ler mais frames
                video->m_videoData.packetQueue.PushFront(packet);
                continue;
            }

            else if (lastError == AVERROR_EOF) {
                //acabaram os packets
                decoding = false;
                continue;
            }

            else if (lastError != 0) {
                // erro qualquer
                char buffer[AV_ERROR_MAX_STRING_SIZE];
                av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
                SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error sending package to video decode: %s", buffer);
                continue;
            }

            }
    }

    av_freep(decodedFrame->data);
    av_frame_free(&decodedFrame);

    video->m_videoData.frameQueue.Push(nullptr);

    av_packet_free(&packet);
    return 0;
}

int AudioDecodeThread(void* userdata) {
    int lastError;
    AudioData* data = static_cast<AudioData*>(userdata);
    Video* video = data->video;
    AVFrame* frame = av_frame_alloc();

    AVPacket* packet;
    bool decoding = true;

    while (true) {
        lastError = avcodec_receive_frame(data->codecContext.get(), frame);

        if (lastError == 0) {
            // recebeu frame
            data->frameQueue.Push(frame);
            frame = av_frame_alloc();
            continue;
        }

        else if (lastError == AVERROR_EOF || video->m_videoDone) {
            // acabou os frames
            data->frameQueue.Push(nullptr);
            SDL_Log("Audio Thread Exit");
            return 0;
        }

        else if (lastError != AVERROR(EAGAIN)) {
            // erro qualquer
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error reciving frame: %s", buffer);
            continue;
        }

        if (decoding) {
            // precisa de mais packets

            packet = data->packetQueue.Get();

            if (packet == nullptr) {
                decoding = false;
            }

            lastError = avcodec_send_packet(data->codecContext.get(), packet);
            if (lastError == 0) {
                // packet recebida com sucesso
                av_packet_free(&packet);
                continue;
            }

            else if (lastError == AVERROR(EAGAIN)) {
                // precisa ler mais frames
                data->packetQueue.PushFront(packet);
                continue;
            }

            else if (lastError == AVERROR_EOF) {
                //acabaram os packets
                decoding = false;
                continue;
            }

            else if (lastError != 0) {
                // erro qualquer
                char buffer[AV_ERROR_MAX_STRING_SIZE];
                av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
                SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error sending package to audio decode: %s", buffer);
                continue;
            }
        }
    }
}