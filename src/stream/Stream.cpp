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
    packetQueue.Push(std::unique_ptr<AVPacket, AVPacketDeleter>(pkt));
}

void Stream::Flush() {
    packetQueue.Push(nullptr);
    frameQueue.Push(nullptr);
}

int Stream::DecoderThread(void* userdata) {
    Stream* stream = static_cast<Stream*>(userdata);
    AVFrame* decodedFrame = av_frame_alloc();
    std::unique_ptr<AVPacket,AVPacketDeleter> packet = nullptr;

    while (!stream->video->m_videoDone) {
        int lastError = avcodec_receive_frame(stream->context.get(), decodedFrame);

        if (lastError == 0) {
            // recebeu frame
            stream->frameQueue.Push(std::unique_ptr<AVFrame,AVFrameDeleter>(decodedFrame));
            decodedFrame = av_frame_alloc();
            continue;
        }

        else if (lastError == AVERROR_EOF) {
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

        // precisa de mais packets
        packet = std::move(stream->packetQueue.Get());

        if (packet == nullptr) {
            // Flush
            avcodec_flush_buffers(stream->context.get());
        }

        // Receber Packets

        lastError = avcodec_send_packet(stream->context.get(), packet.get());
        if (lastError == 0) {
            // packet recebida com sucesso
            continue;
        }

        else if (lastError == AVERROR(EAGAIN)) {
            // precisa ler mais frames
            stream->packetQueue.PushFront(std::move(packet));
            SDL_Delay(1);
            continue;
        }

        else if (lastError == AVERROR_EOF) {
            //acabaram os packets
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

    av_frame_free(&decodedFrame);

    stream->frameQueue.Push(nullptr);

    return 0;
}