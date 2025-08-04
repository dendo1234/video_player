#include <video/Video.hpp>
#include <video/Decoders.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

int PacketReaderThread(void* userdata) {
    Video* video = (Video*)userdata;
    AVPacket* packet = av_packet_alloc();

    while (av_read_frame(video->m_formatContext.get(), packet) == 0 && !video->m_videoDone) {
        if (packet->stream_index == video->videoStreamIndex) {
            video->m_videoData.packetQueue.Push(packet);
        }
        else if (packet->stream_index == video->audioStreamIndex) {
            video->m_audioData.packetQueue.Push(packet);
        }
        packet = av_packet_alloc();
    }
    // Leitura de packetes finalizada


    video->m_videoData.packetQueue.Push(nullptr);
    video->m_audioData.packetQueue.Push(nullptr);
    av_packet_free(&packet);
    return 0;
}


int Video::GetFormatContext(const char* filename) {
    int lastError;

    AVFormatContext* avFormatContextRaw = nullptr;

    lastError = avformat_open_input(&avFormatContextRaw, filename, nullptr, nullptr);
    if(lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error opening video file: %s", buffer);
        return 1;
    }
    m_formatContext = unique_ptr<AVFormatContext, AVFormatContextDeleter>(avFormatContextRaw);

    lastError = avformat_find_stream_info(m_formatContext.get(), nullptr);
    if(lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error reading video stream: %s", buffer);
        return 1;
    }

    #ifndef NDEBUG
    // av_dump_format(m_formatContext.get(), 0, filename, 0);
    #endif
    return 0;
}

Video::Video(const char* filename, SDL_Renderer* renderer) 
    : renderer{renderer} {
    int lastError;
    lastError = GetFormatContext(filename);
    if (lastError != 0) {
        return;
    }

    // SwsContext* swsContext = sws_getContext(codecContext->width, codecContext->height,
    //         codecContext->pix_fmt, codecContext->width, codecContext->height,
    //         AV_PIX_FMT_YUV420P,
    //         SWS_BILINEAR,
    //         NULL,
    //         NULL,
    //         NULL);

    // Find the first video stream
    for(unsigned i = 0; i < m_formatContext->nb_streams; i++) {
        if(m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoData.time_base = m_formatContext->streams[i]->time_base;
            videoStreamIndex = i;
            continue;
        }

        if(m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioData.time_base = m_formatContext->streams[i]->time_base;
            audioStreamIndex = i;
            continue;
        }
    }


    const AVCodec* videoCodec = avcodec_find_decoder(m_formatContext->streams[videoStreamIndex]->codecpar->codec_id);
    if (videoCodec == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Unsuported video decoder: %s", avcodec_get_name(m_formatContext->streams[videoStreamIndex]->codecpar->codec_id));
        return;
    }

    const AVCodec* audioCodec = avcodec_find_decoder(m_formatContext->streams[audioStreamIndex]->codecpar->codec_id);
    if (audioCodec == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Unsuported audio decoder: %s", avcodec_get_name(m_formatContext->streams[videoStreamIndex]->codecpar->codec_id));
        return;
    }


    AVCodecContext* avCodecContexRaw = avcodec_alloc_context3(videoCodec);
    if (avCodecContexRaw == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error allocating codec contexts");
        return;
    }
    m_videoData.m_videoCodecContext = unique_ptr<AVCodecContext, AVCodecContextDeleter>(avCodecContexRaw);

    avCodecContexRaw = avcodec_alloc_context3(audioCodec);
    if (avCodecContexRaw == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error allocating codec contexts");
        return;
    }
    m_audioData.codecContext = unique_ptr<AVCodecContext, AVCodecContextDeleter>(avCodecContexRaw);

    
    lastError = avcodec_parameters_to_context(m_videoData.m_videoCodecContext.get(), m_formatContext->streams[videoStreamIndex]->codecpar);
    if (lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error codec: %s", buffer);
    }

    lastError = avcodec_parameters_to_context(m_audioData.codecContext.get(), m_formatContext->streams[audioStreamIndex]->codecpar);
    if (lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error codec: %s", buffer);
    }
    

    lastError = avcodec_open2(m_videoData.m_videoCodecContext.get(), videoCodec, nullptr);
    if (lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error initializing codecContex to use given codec: %s", buffer);
    }

    lastError = avcodec_open2(m_audioData.codecContext.get(), audioCodec, nullptr);
    if (lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error initializing codecContex to use given codec: %s", buffer);
    }


    m_videoData.m_swsContext = unique_ptr<SwsContext, SwsContextDeleter>(sws_getContext(m_videoData.m_videoCodecContext->width, m_videoData.m_videoCodecContext->height, m_videoData.m_videoCodecContext->pix_fmt,
                                    m_videoData.m_videoCodecContext->width, m_videoData.m_videoCodecContext->height, AV_PIX_FMT_YUV420P12,
                                    SWS_BICUBIC, nullptr, nullptr, nullptr));

    if (m_videoData.m_swsContext == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error initializing SwsContext");
    }

    
    m_audioData.audioSpec.freq = m_audioData.codecContext->sample_rate;
    m_audioData.audioSpec.format = SDL_AUDIO_S16;
    m_audioData.audioSpec.channels = static_cast<uint8_t>(m_audioData.codecContext->ch_layout.nb_channels);

    // SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &m_audioData.audioSpec, AudioCallback, this);
    m_audioDevideID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &m_audioData.audioSpec); 
    m_audioStream = SDL_CreateAudioStream(&m_audioData.audioSpec, &m_audioData.audioSpec);
    if (m_audioStream == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create audio stream: %s", SDL_GetError());
    }

    if (!SDL_BindAudioStream(m_audioDevideID, m_audioStream)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to bind audio stream: %s", SDL_GetError());
    }

    if (!SDL_ResumeAudioDevice(m_audioDevideID)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to resume audio device: %s", SDL_GetError());
    }



    // SDL_AudioStream* audioStream = SDL_CreateAudioStream(&m_audioData.audioSpec, &m_audioData.audioSpec);
    // SDL_SetAudioStreamGetCallback(audioStream, AudioCallback, this);

    // SDL_BindAudioStream(m_audioDevideID, audioStream);

    // SDL_ResumeAudioDevice(m_audioDevideID);

    AVChannelLayout inChannelLayout;
    av_channel_layout_default(&inChannelLayout, m_audioData.audioSpec.channels);
    SwrContext *swr = nullptr;
    lastError = swr_alloc_set_opts2(&swr, &inChannelLayout, AV_SAMPLE_FMT_S16, m_audioData.audioSpec.freq, &m_audioData.codecContext->ch_layout, m_audioData.codecContext->sample_fmt, m_audioData.codecContext->sample_rate, 0, nullptr);
    if (lastError != 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error alloc swr: %s", buffer);
        return;
    }
    lastError = swr_init(swr);
    if (lastError != 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error initing swr: %s", buffer);
        return;
    }
    m_audioData.swrContext = unique_ptr<SwrContext, SwrContextDeleter>(swr);

    
     
    // initialize VideoData
    m_videoData.texture = unique_ptr<SDL_Texture, SDL_TextureDeleter>(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, m_videoData.m_videoCodecContext->width, m_videoData.m_videoCodecContext->height));
    m_videoData.mutex = SDL_CreateMutex();
    m_videoData.cond = SDL_CreateCondition();

    InitializeThreads();

    // seta o framerate do jogo para ser o framerate do vídeo
    [[maybe_unused]] int framerate = static_cast<int>((double)m_videoData.m_videoCodecContext->framerate.den * 1000 / m_videoData.m_videoCodecContext->framerate.num);
}

Video::~Video() {
    m_videoData.packetQueue.Push(nullptr);
    m_audioData.packetQueue.Push(nullptr);
    m_videoData.frameQueue.Push(nullptr);
    m_audioData.frameQueue.Push(nullptr);
    SDL_CloseAudioDevice(m_audioDevideID);
    int status;
    SDL_WaitThread(m_packageReader, &status);
    SDL_WaitThread(m_videoDecoder, &status);

    // SDL_Log("video framequeue size: %d", m_videoData.frameQueue.Size());
    // SDL_Log("audio framequeue size: %d", m_audioData.frameQueue.Size());

    m_videoData.frameQueue.Push(nullptr);
    m_audioData.frameQueue.Push(nullptr);

	AVFrame* f;
	while ((f = m_videoData.frameQueue.Get()) != nullptr) {
		av_frame_free(&f);
	}
	while ((f = m_audioData.frameQueue.Get()) != nullptr) {
		av_frame_free(&f);
	}
}

void Video::InitializeThreads() {
    m_packageReader = SDL_CreateThread(PacketReaderThread, "Package reader", (void*)this);
    m_videoDecoder = SDL_CreateThread(VideoDecodeThread, "Video decoder", (void*)this);
    m_audioDecoder = SDL_CreateThread(AudioDecodeThread, "Audio decoder", (void*)this);
}

void Video::Start() {
    m_startTick = SDL_GetTicks();
}

double Video::GetSyncClock() {
    return clock;
}

void Video::Update(uint64_t dt) {
    if (m_videoDone == true) {
        return;
    }

    clock += dt/1000.0;
    SDL_SignalCondition(m_videoData.cond);


    SDL_Log("%u", SDL_GetAudioStreamQueued(m_audioStream));
    AVFrame* frame = m_audioData.frameQueue.GetBeforePts(static_cast<int64_t>(GetSyncClock() / av_q2d(m_audioData.time_base)));
    if (frame == nullptr) {
        // memset(stream, 0, total_amount);
        // video->m_videoDone = true;
    } else {
        m_audioData.clock = frame->pts * av_q2d(m_audioData.time_base);
        if (!SDL_PutAudioStreamData(m_audioStream, frame->data[0], frame->linesize[0])) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to put audio stream data: %s", SDL_GetError());
        }
    }
    SDL_Log("%u", SDL_GetAudioStreamQueued(m_audioStream));
    

    av_frame_free(&frame);

    frame = m_videoData.frameQueue.GetBeforePts(static_cast<int64_t>(GetSyncClock() / av_q2d(m_videoData.time_base)));
    if (frame != nullptr) {
        SDL_UpdateYUVTexture(
                m_videoData.texture.get(),
                nullptr,
                frame->data[0],
                frame->linesize[0],
                frame->data[1],
                frame->linesize[1],
                frame->data[2],
                frame->linesize[2]
            );
		av_frame_free(&frame);
   //     if (frame != nullptr) {
			//av_frame_unref(frame);
   //     }
    } 
}

void Video::Render() const {
    bool lastError = SDL_RenderTexture(renderer, m_videoData.texture.get(), nullptr, nullptr);
    if (lastError == false) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error rendering video texture: %s", SDL_GetError());
    }
    // SDL_UnlockMutex(m_videoData.mutex);


}



