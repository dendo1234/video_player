#include <video/Video.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

int Video::PacketReaderThread(void* userdata) {
    Video* video = (Video*)userdata;
    AVPacket* packet;

    while (packet = av_packet_alloc(), av_read_frame(video->mediaFile.GetFormatContext(), packet) == 0 && !video->m_videoDone) {
        if (packet->stream_index == video->videoStream.GetStreamIndex()) {
            video->videoStream.PushPacket(packet);
            continue;
        }
        for (auto& audioStream : video->audioStreams) {
            if (packet->stream_index == audioStream.GetStreamIndex()) {
                audioStream.PushPacket(packet);
                break;
            }
        }
    }
        
    // Leitura de packetes finalizada
    video->videoStream.PushPacket(nullptr);
    for (auto& audioStream : video->audioStreams) {
        audioStream.PushPacket(nullptr);
    }
    
    av_packet_free(&packet);
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
    videoStream.Flush();

    for (auto& audioStream : audioStreams) {
        audioStream.Flush();
    }
    

    SDL_CloseAudioDevice(m_audioDevideID);
    // int status;
    // SDL_WaitThread(m_packageReader, &status);
    // SDL_WaitThread(m_videoDecoder, &status);
    // for (auto& audioData : audios) {
    //     SDL_WaitThread(audioData.audioDecoder, &status);
    //     SDL_WaitThread(audioData.audioConsumer, &status);
    // }
    
    // TODO: Verify if this is necessary
    // I supose not but we will see
    // m_videoData.frameQueue.Push(nullptr);
    // for (auto& audioData : audios) {
    //     audioData.packetQueue.Push(nullptr);
    //     audioData.frameQueue.Push(nullptr);
    // }

    // TODO: transfer cleanup to the Deque class
	// AVFrame* f;
	// while ((f = m_videoData.frameQueue.Get()) != nullptr) {
	// 	av_frame_free(&f);
	// }
    // for (auto& audioData : audios) {
    //     while ((f = audioData.frameQueue.Get()) != nullptr) {
    //         av_frame_free(&f);
    //     }
    // }
}

void Video::InitializeThreads() {
    m_packageReader = SDL_CreateThread(PacketReaderThread, "Package reader", (void*)this);
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

void Video::Update(uint64_t dt) {
    if (m_videoDone == true) {
        return;
    }

    clock.UpdateDt(dt/1e9);


    AVFrame* frame = videoStream.GetFrameBeforePts(static_cast<int64_t>(GetSyncClock() / av_q2d(videoStream.GetTimeBase())));
    if (frame != nullptr) {
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
		av_frame_free(&frame);
    } 
}

void Video::Render() const {
    bool lastError = SDL_RenderTexture(renderer, texture.get(), nullptr, nullptr);
    if (lastError == false) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error rendering video texture: %s", SDL_GetError());
    }
}
