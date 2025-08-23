#include <video/Video.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

#include "imgui.h"

using namespace std;

void Video::ReadPacket(Video* video) {
    AVPacket* packet = av_packet_alloc();
    int lastError = av_read_frame(video->mediaFile.GetFormatContext(), packet);

    if (lastError == 0) {
        if (packet->stream_index == video->videoStream.GetStreamIndex()) {
            video->videoStream.PushPacket(packet);
            return;
        }
        for (auto& audioStream : video->audioStreams) {
            if (packet->stream_index == audioStream.GetStreamIndex()) {
                audioStream.PushPacket(packet);
                return;
            }
        }
        // Packet is in a stream that is not loaded
        av_packet_free(&packet);
    } else if (lastError == AVERROR_EOF) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Packet Reader EOF: %s", buffer);
        av_packet_free(&packet);
        return;
    } else {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Packet Reader error: %s", buffer);
        av_packet_free(&packet);
        return;
    }
}

int Video::PacketReaderThread(void* userdata) {
    Video* video = (Video*)userdata;
    int numberOfStreams = video->mediaFile.GetNumberOfStreams();

    while (!video->m_videoDone) {
        if (video->seekInterface.seekRequested) {
            SDL_Log("Inciando seek para %f", video->seekInterface.timestamp);
            uint64_t tic = SDL_GetTicksNS();
            video->clock.SetSeeking(true);
            video->Seek(video->seekInterface.timestamp);
            video->seekInterface.seekRequested = false;

            for (int i = 0; i < numberOfStreams; i++) {
                ReadPacket(video);
            }
            video->clock.SetSeeking(false);
            uint64_t tac = SDL_GetTicksNS();
            double tempo = (tac - tic) /1e6;
            SDL_Log("Terminando seek, milisegundos: %f", tempo);
        }

        ReadPacket(video);
    }
    
    return 0;
}

unique_ptr<AVCodecContext, AVCodecContextDeleter> Video::InitializeDecoder(int streamIndex) {
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

    int lastError = avcodec_parameters_to_context(context.get(), parameters);
    if (lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error codec: %s", buffer);
    }

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


Video::Video(const char* filename, SDL_Renderer* renderer) 
    : mediaFile(filename),
    videoStream{InitializeVideoStream()},
    m_audioDevideID{SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr)},
    audioStreams{InitializeAudioStreams()},
    renderer{renderer} {
    
    texture = unique_ptr<SDL_Texture, SDL_TextureDeleter>(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, videoStream.GetWidth(), videoStream.GetHeight()));

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
    packageReader = unique_ptr<SDL_Thread, SDL_ThreadDeleter>(SDL_CreateThread(PacketReaderThread, "Package reader", (void*)this));
}

void Video::FlushStreams() {
    videoStream.Flush();
    for (auto &&audioStream : audioStreams) {
        audioStream.Flush();
    }
}

void Video::Start() {
    m_startTick = SDL_GetTicks();
}

double Video::GetSyncClock() {
    return clock.GetTime();
}

void Video::Seek(double timestamp) {
    int flags = timestamp > clock.GetTime() ? 0 : AVSEEK_FLAG_BACKWARD;
    mediaFile.Seek(timestamp, flags);
    FlushStreams();
    clock.UpdateTime(timestamp);
}

void Video::RequestSeek(double timestamp) {
    seekInterface.seekRequested = true;
    seekInterface.timestamp = timestamp;
}

void Video::GuiPass() {
    ImGui::Text("Clock: %f", clock.GetTime());
    videoStream.GuiPass();
    for (auto &&stream : audioStreams) {
        stream.GuiPass();
    }
}

void Video::Update(uint64_t dt) {
    if (m_videoDone == true) {
        return;
    }

    clock.UpdateDt(dt/1e9);


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
        SDL_Log("Textura atualizada, clock: %f", GetSyncClock());
    } 
}

void Video::Render() const {
    bool lastError = SDL_RenderTexture(renderer, texture.get(), nullptr, nullptr);
    if (lastError == false) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error rendering video texture: %s", SDL_GetError());
    }
}
