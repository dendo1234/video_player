#include <video/Video.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

using namespace std;


ReadPacketReturn Video::ReadPacket(Video* video) {
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to av_packet_alloc");
        return ReadPacketReturn{.status = ReadPacketReturn::Status::FAILURE};
    }
    
    int lastError = av_read_frame(video->mediaFile.GetFormatContext(), packet);

    if (lastError == 0) {
        ReadPacketReturn ret{.status = ReadPacketReturn::Status::SUCESS,
            .streamIndex = packet->stream_index,
            .pts = packet->pts };
        if (packet->stream_index == video->videoStream.GetStreamIndex()) {
            video->videoStream.PushPacket(packet);
            return ret;
        }
        for (auto& audioStream : video->audioStreams) {
            if (packet->stream_index == audioStream.GetStreamIndex()) {
                audioStream.PushPacket(packet);
                return ret;
            }
        }
        // Packet is in a stream that is not loaded
        av_packet_free(&packet);
    } else if (lastError == AVERROR_EOF) {
        av_packet_free(&packet);
        return ReadPacketReturn{.status = ReadPacketReturn::Status::END};
    } else {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Packet Reader error: %s", buffer);
        av_packet_free(&packet);
        return ReadPacketReturn{.status = ReadPacketReturn::Status::FAILURE};
    }
    av_packet_free(&packet);
    return ReadPacketReturn{.status = ReadPacketReturn::Status::FAILURE};
}

int Video::PacketReaderThread(void* userdata) {
    Video* video = (Video*)userdata;
    int videoStreamIndex = video->videoStream.GetStreamIndex();

    while (!video->m_videoDone) {
        if (video->seekInterface.seekRequested.load(memory_order::acquire)) {
            video->clock.SetSeeking(true);
            video->Seek(video->seekInterface.targetTimestamp, video->seekInterface.delta);

            ReadPacketReturn packetData;

            do {
                packetData = ReadPacket(video);
                if (packetData.status != ReadPacketReturn::Status::SUCESS) {
                    SDL_Delay(10);
                    continue;
                }
            } while (packetData.streamIndex != videoStreamIndex);

            double timestamp = packetData.pts*av_q2d(video->mediaFile.GetTimeBase(videoStreamIndex));
            // video's I-frame (keyframe) timestamps drives the sync clock 
            video->clock.UpdateTime(timestamp);
            
            video->seekInterface.seekRequested.store(false, memory_order::relaxed);
            video->clock.SetSeeking(false);
        }

        auto ret = ReadPacket(video);
        if (ret.status == ReadPacketReturn::Status::END) {
            SDL_Delay(10);
        }
    }
    
    return 0;
}

AVPixelFormat GetFormat([[maybe_unused]] AVCodecContext *s, const AVPixelFormat * fmt) {
    auto _ = fmt;
    return AV_PIX_FMT_VULKAN;
}

void Video::TryHardwareDecode(const AVCodec* codec, AVCodecContext* context) {
    AVHWDeviceType type = AV_HWDEVICE_TYPE_VULKAN;

    for (int i = 0;;i++) {
        const AVCodecHWConfig* hwconfig = avcodec_get_hw_config(codec, i);
        if (hwconfig == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error codec does not support hardware decode");
            return;
        }

        if (hwconfig->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
            hwconfig->device_type == type) {
            auto _ = hwconfig->pix_fmt;
            break;
        }
    }

    AVBufferRef* ref;
    int lastError = av_hwdevice_ctx_create(&ref, AV_HWDEVICE_TYPE_VULKAN, nullptr, nullptr, 0);
    if (lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error codec: %s", buffer);
    }

    context->get_format = GetFormat;
    context->hw_device_ctx = av_buffer_ref(ref);
}

unique_ptr<AVCodecContext, AVCodecContextDeleter> Video::InitializeDecoder(int streamIndex) {
    int lastError;
    const AVCodecParameters* parameters = mediaFile.GetCodecParameters(streamIndex);

    const AVCodec* codec = avcodec_find_decoder(parameters->codec_id);
    if (codec == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Unsuported codec: %s", avcodec_get_name(parameters->codec_id));
    }

    AVCodecContext* avCodecContexRaw = avcodec_alloc_context3(codec);
    if (avCodecContexRaw == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error allocating codec contexts");
    }

    unique_ptr<AVCodecContext, AVCodecContextDeleter> context(avCodecContexRaw);

    lastError = avcodec_parameters_to_context(context.get(), parameters);
    if (lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error codec: %s", buffer);
    }

    // TryHardwareDecode(codec, context.get());

    lastError = avcodec_open2(context.get(), codec, nullptr);
    if (lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error initializing codecContex to use given codec: %s", buffer);
    }

    return context;
}

VideoStream Video::InitializeVideoStream() {
    int videoStreamIndex = mediaFile.GetVideoStreamIndex();
    return VideoStream(this, InitializeDecoder(videoStreamIndex), videoStreamIndex, mediaFile.GetTimeBase(videoStreamIndex));
}

std::vector<AudioStream> Video::InitializeAudioStreams() {
    vector<int> audioStreamIndicies = mediaFile.GetAudioStreamIndices();
    std::vector<AudioStream> vector;
    vector.reserve(audioStreamIndicies.size());

    for (auto &&index : audioStreamIndicies) {
        vector.emplace_back(this, InitializeDecoder(index), index, mediaFile.GetTimeBase(index));
    }
    return vector;
}


Video::Video(const Layer& layer, const char* filename)
    : Layer{layer},
      mediaFile(filename),
    videoStream{InitializeVideoStream()},
    m_audioDevideID{SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr)},
    audioStreams{InitializeAudioStreams()} {
    
    texture = windowTarget->CreateTexture(SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, videoStream.GetWidth(), videoStream.GetHeight());
    SDL_Texture* previousTarget = SDL_GetRenderTarget(windowTarget->GetRenderer());
    SDL_SetRenderTarget(windowTarget->GetRenderer(), texture.get());
    SDL_SetRenderLogicalPresentation(windowTarget->GetRenderer(), videoStream.GetWidth(), videoStream.GetHeight(), SDL_RendererLogicalPresentation::SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderTarget(windowTarget->GetRenderer(), previousTarget);

    InitializeThreads();

    // seta o framerate do jogo para ser o framerate do vídeo
    // [[maybe_unused]] int framerate = static_cast<int>((double)m_videoData.m_videoCodecContext->framerate.den * 1000 / m_videoData.m_videoCodecContext->framerate.num);
}

// TODO: make more organized destructor
Video::~Video() {
    m_videoDone = true;

    SDL_CloseAudioDevice(m_audioDevideID);
    FlushStreams();
}

void Video::InitializeThreads() {
    packageReader = std::jthread(PacketReaderThread, (void*)this);
    packageReader.detach();
}

void Video::FlushStreams() {
    videoStream.Flush();
    for (auto &&audioStream : audioStreams) {
        audioStream.Flush();
    }
}

int Video::GetVideoWidth() {
    return videoStream.GetWidth();
}
int Video::GetVideoHeight() {
    return videoStream.GetHeight();
}

double Video::GetSyncClock() {
    return clock.GetTime();
}

void Video::Seek(double targetTimestamp, double delta) {
    mediaFile.Seek(targetTimestamp, delta);
    FlushStreams();
}

void Video::RequestSeek(double targetTimestamp, double delta) {
    seekInterface.targetTimestamp = targetTimestamp;
    seekInterface.delta = delta;
    seekInterface.seekRequested.store(true, memory_order::release);
}

void Video::TogglePause() {
    clock.TogglePaused();
}

void Video::GuiPass() {

}

double Video::GetPlaybackPercentage() const {
    return clock.GetTime()/mediaFile.GetDuration();
}

double Video::ConvertPercentageToTimestamp(double percentage) const {
    return percentage*mediaFile.GetDuration();
}

void Video::OnUpdate(double dt) {
    if (m_videoDone == true) {
        return;
    }

    clock.UpdateDt(dt);


    std::unique_ptr<AVFrame,AVFrameDeleter> frame = videoStream.GetFrameBeforePts(static_cast<int64_t>(GetSyncClock() / av_q2d(videoStream.GetTimeBase())));
    if (frame) {
        // TODO: Implement video clock
        // m_videoData.clock = frame->pts * av_q2d(m_videoData.time_base);
        SDL_UpdateYUVTexture(
                texture.get(),
                nullptr,
                frame->data[0],
                frame->linesize[0],
                frame->data[1],
                frame->linesize[1],
                frame->data[2],
                frame->linesize[2]
            );
    } 
}

SDL_FRect Video::CalculateDstBox() {
    int windowWidth;
    int windowHeight;
    windowTarget->GetWindowSize(windowWidth, windowHeight);

    // todo: get from somewhere else
    int videoUIHeight = 18;

    windowHeight -= videoUIHeight;

    int videoWidth = videoStream.GetWidth();
    int videoHeight = videoStream.GetHeight();

    float videoAspect = static_cast<float>(videoWidth)/videoHeight;
    float windowAspect = static_cast<float>(windowWidth)/windowHeight;

    //case 1: window aspect is bigger:
    if (windowAspect > videoAspect) {
        // blackbars on the left/right
        float w = videoAspect*windowHeight;
        return SDL_FRect{
            .x = (windowWidth-w)/2,
            .y = 0.0f,
            .w = w,
            .h = static_cast<float>(windowHeight)
        };
    } else {
        float h = 1/videoAspect*windowWidth;
        return SDL_FRect {
            .x = 0.0f,
            .y = (windowHeight-h)/2,
            .w = static_cast<float>(windowWidth),
            .h = h
        };
    }
}

void Video::OnRender() {
    bool lastError = SDL_RenderTexture(windowTarget->GetRenderer(), texture.get(), nullptr, &dstBox);
    if (lastError == false) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error rendering video texture: %s", SDL_GetError());
    }
}

EventResult Video::OnEvent(const Event& event) {

    switch (event.sdl_event->type)
    {
    case SDL_EVENT_WINDOW_RESIZED: {
        dstBox = CalculateDstBox();
        break;
    }
    case SDL_EVENT_KEY_DOWN: {
        SDL_KeyboardEvent& keyEvent = event.sdl_event->key;
        switch (keyEvent.key) {
        case SDLK_RIGHT:
            RequestSeek(GetSyncClock() + 5, 5);
            break;
        case SDLK_LEFT:
            RequestSeek(GetSyncClock() - 5, -5);
            break;
        case SDLK_L:
            RequestSeek(GetSyncClock() + 10, 10);
            break;
        case SDLK_J:
            RequestSeek(GetSyncClock() - 10, -10);
            break;
        case SDLK_SPACE:
            TogglePause();
        default:
            break;
        }
    }
    }
    return EventResult::Continue;
}