#include <stream/Stream.hpp>
#include <video/Video.hpp>

Stream::Stream() {

}

Stream::Stream(Video* video, std::unique_ptr<AVCodecContext, AVCodecContextDeleter> context, int streamIndex, AVRational timeBase) 
    : video{video},
      context{std::move(context)},
      streamIndex{streamIndex},
      timeBase{timeBase} {

}

void Stream::PushPacket(AVPacket* pkt) {
    packetQueue.Push(pkt);
}

void Stream::Flush() {
    packetQueue.Push(nullptr);
    frameQueue.Push(nullptr);
}

int Stream::DecoderThread(void* userdata) {
    Stream* stream = static_cast<Stream*>(userdata);
    AVFrame* decodedFrame = av_frame_alloc();
    AVPacket* packet = nullptr;
    bool decoding = true;

    while (true) {
        int lastError = avcodec_receive_frame(stream->context.get(), decodedFrame);

        if (lastError == 0) {
            // recebeu frame
            stream->frameQueue.Push(decodedFrame);
            decodedFrame = av_frame_alloc();
            continue;
        }

        else if (lastError == AVERROR_EOF || stream->video->m_videoDone) {
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
            packet = stream->packetQueue.Get();

            if (packet == nullptr) {
                decoding = false;
            }

            lastError = avcodec_send_packet(stream->context.get(), packet);
            if (lastError == 0) {
                // packet recebida com sucesso
                if (packet != nullptr) {
                    av_packet_unref(packet);
                }
                continue;
            }

            else if (lastError == AVERROR(EAGAIN)) {
                // precisa ler mais frames
                stream->packetQueue.PushFront(packet);
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

    stream->frameQueue.Push(nullptr);

    if (packet != nullptr) {
        av_packet_free(&packet);
    }
    return 0;
}