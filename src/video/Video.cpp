#include <video/Video.hpp>
#include <video/Decoders.hpp>
#include <video/AudioConsumer.hpp>
#include <video/AudioData.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

int PacketReaderThread(void* userdata) {
    Video* video = (Video*)userdata;
    AVPacket* packet;

    while (packet = av_packet_alloc(), av_read_frame(video->m_formatContext.get(), packet) == 0 && !video->m_videoDone) {
        if (packet->stream_index == video->videoStreamIndex) {
            video->m_videoData.packetQueue.Push(packet);
            continue;
        }
        for (auto& audioData : video->audios) {
            if (packet->stream_index == audioData.streamIndex) {
                audioData.packetQueue.Push(packet);
                break;
            }
        }
    }
        
    // Leitura de packetes finalizada
    video->m_videoData.packetQueue.Push(nullptr);
    for (auto& audioData : video->audios) {
        audioData.packetQueue.Push(nullptr);
    }
    
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
    av_dump_format(m_formatContext.get(), 0, filename, 0);
    #endif
    return 0;
}

void Video::InitializeStreamsIndexes() {
    // Find the first video stream
    for(unsigned i = 0; i < m_formatContext->nb_streams; i++) {
        if(m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (videoStreamIndex != -1) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "There is more than one video stream in file? Thats undefined!");
            }
            m_videoData.time_base = m_formatContext->streams[i]->time_base;
            videoStreamIndex = i;
            continue;
        }

        if(m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            AudioData& audioData = audios.emplace_back(this);

            audioData.time_base = m_formatContext->streams[i]->time_base;
            audioData.streamIndex = i;

            numberOfAudioStreams++;
            continue;
        }
    }

}

int Video::InitializeDecoders() {
    // Video
    const AVCodec* videoCodec = avcodec_find_decoder(m_formatContext->streams[videoStreamIndex]->codecpar->codec_id);
    if (videoCodec == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Unsuported video decoder: %s", avcodec_get_name(m_formatContext->streams[videoStreamIndex]->codecpar->codec_id));
        return 1;
    }

    AVCodecContext* avCodecContexRaw = avcodec_alloc_context3(videoCodec);
    if (avCodecContexRaw == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error allocating codec contexts");
        return 1;
    }
    m_videoData.m_videoCodecContext = unique_ptr<AVCodecContext, AVCodecContextDeleter>(avCodecContexRaw);

    int lastError = avcodec_parameters_to_context(m_videoData.m_videoCodecContext.get(), m_formatContext->streams[videoStreamIndex]->codecpar);
    if (lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error codec: %s", buffer);
        return 1;
    }

    lastError = avcodec_open2(m_videoData.m_videoCodecContext.get(), videoCodec, nullptr);
    if (lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error initializing codecContex to use given codec: %s", buffer);
    }

    // Audio
    for (auto& audioData : audios) {
        const AVCodec* audioCodec = avcodec_find_decoder(m_formatContext->streams[audioData.streamIndex]->codecpar->codec_id);
        if (audioCodec == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Unsuported audio decoder: %s", avcodec_get_name(m_formatContext->streams[videoStreamIndex]->codecpar->codec_id));
            return 1;
        }

        avCodecContexRaw = avcodec_alloc_context3(audioCodec);
        if (avCodecContexRaw == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error allocating codec contexts");
            return 1;
        }
        audioData.codecContext = unique_ptr<AVCodecContext, AVCodecContextDeleter>(avCodecContexRaw);

        lastError = avcodec_parameters_to_context(audioData.codecContext.get(), m_formatContext->streams[audioData.streamIndex]->codecpar);
        if (lastError < 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error codec: %s", buffer);
        }

        lastError = avcodec_open2(audioData.codecContext.get(), audioCodec, nullptr);
        if (lastError < 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error initializing codecContex to use given codec: %s", buffer);
        }
    }
    return 0;
}

void Video::InitializeAudioStreams() {
    for (auto& audioData : audios) {
        audioData.audioSpec.freq = audioData.codecContext->sample_rate;
        audioData.audioSpec.format = SDL_AUDIO_S16;
        audioData.audioSpec.channels = static_cast<uint8_t>(audioData.codecContext->ch_layout.nb_channels);

        SDL_GetAudioDeviceFormat(m_audioDevideID, &audioData.outputAudioSpec, nullptr);
        audioData.m_audioStream = SDL_CreateAudioStream(&audioData.audioSpec, &audioData.outputAudioSpec);
        if (audioData.m_audioStream == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create audio stream: %s", SDL_GetError());
        }

        if (!SDL_BindAudioStream(m_audioDevideID, audioData.m_audioStream)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to bind audio stream: %s", SDL_GetError());
        }

        if (!SDL_ResumeAudioDevice(m_audioDevideID)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to resume audio device: %s", SDL_GetError());
        }
    }
}

int Video::InitializeSwrContexts() {
    for (auto& audioData : audios) {
        AVChannelLayout inChannelLayout;
        av_channel_layout_default(&inChannelLayout, audioData.audioSpec.channels);
        SwrContext *swr = nullptr;
        int lastError = swr_alloc_set_opts2(&swr, &inChannelLayout, AV_SAMPLE_FMT_S16, audioData.audioSpec.freq, &audioData.codecContext->ch_layout, audioData.codecContext->sample_fmt, audioData.codecContext->sample_rate, 0, nullptr);
        if (lastError != 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error alloc swr: %s", buffer);
            return 1;
        }
        lastError = swr_init(swr);
        if (lastError != 0) {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error initing swr: %s", buffer);
            return 1;
        }
        audioData.swrContext = unique_ptr<SwrContext, SwrContextDeleter>(swr);
    }
    return 0;
}

Video::Video(const char* filename, SDL_Renderer* renderer) 
    : renderer{renderer} {
    
    int lastError = GetFormatContext(filename);
    if (lastError != 0) {
        return;
    }

    InitializeStreamsIndexes();

    lastError = InitializeDecoders();
    if (lastError != 0) {
        return;
    }


    m_videoData.m_swsContext = unique_ptr<SwsContext, SwsContextDeleter>(sws_getContext(m_videoData.m_videoCodecContext->width, m_videoData.m_videoCodecContext->height, m_videoData.m_videoCodecContext->pix_fmt,
                                    m_videoData.m_videoCodecContext->width, m_videoData.m_videoCodecContext->height, AV_PIX_FMT_YUV420P12,
                                    SWS_BICUBIC, nullptr, nullptr, nullptr));

    if (m_videoData.m_swsContext == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error initializing SwsContext");
    }


    m_audioDevideID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr); 

    InitializeAudioStreams();
    
    lastError = InitializeSwrContexts();
    if (lastError != 0) {
        return;
    }
     
    // initialize VideoData
    m_videoData.texture = unique_ptr<SDL_Texture, SDL_TextureDeleter>(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, m_videoData.m_videoCodecContext->width, m_videoData.m_videoCodecContext->height));
    m_videoData.mutex = SDL_CreateMutex();
    m_videoData.cond = SDL_CreateCondition();

    InitializeThreads();

    // seta o framerate do jogo para ser o framerate do vídeo
    [[maybe_unused]] int framerate = static_cast<int>((double)m_videoData.m_videoCodecContext->framerate.den * 1000 / m_videoData.m_videoCodecContext->framerate.num);
}

// TODO: make more organized destructor
Video::~Video() {
    m_videoDone = true;
    m_videoData.packetQueue.Push(nullptr);
    m_videoData.frameQueue.Push(nullptr);

    for (auto& audioData : audios) {
        audioData.packetQueue.Push(nullptr);
        audioData.frameQueue.Push(nullptr);

    }
    

    SDL_CloseAudioDevice(m_audioDevideID);
    int status;
    SDL_WaitThread(m_packageReader, &status);
    SDL_WaitThread(m_videoDecoder, &status);
    for (auto& audioData : audios) {
        SDL_WaitThread(audioData.audioDecoder, &status);
        SDL_WaitThread(audioData.audioConsumer, &status);
    }
    

    // TODO: Verify if this is necessary
    m_videoData.frameQueue.Push(nullptr);
    for (auto& audioData : audios) {
        audioData.packetQueue.Push(nullptr);
        audioData.frameQueue.Push(nullptr);
    }

	AVFrame* f;
	while ((f = m_videoData.frameQueue.Get()) != nullptr) {
		av_frame_free(&f);
	}
    for (auto& audioData : audios) {
        while ((f = audioData.frameQueue.Get()) != nullptr) {
            av_frame_free(&f);
        }
    }
}

void Video::InitializeThreads() {
    m_packageReader = SDL_CreateThread(PacketReaderThread, "Package reader", (void*)this);
    m_videoDecoder = SDL_CreateThread(VideoDecodeThread, "Video decoder", (void*)this);
    for (auto& audioData : audios) {
        char audioDecoderName[32] = "Audio Decoder ";
        itoa(audioData.streamIndex, audioDecoderName+14,10);
        char audioConsumerName[32] = "Audio Consumer ";
        itoa(audioData.streamIndex, audioDecoderName+15,10);
        audioData.audioDecoder = SDL_CreateThread(AudioDecodeThread, audioDecoderName, static_cast<void*>(&audioData));
        audioData.audioConsumer = SDL_CreateThread(AudioConsumerThread, audioConsumerName, static_cast<void*>(&audioData));
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
    SDL_SignalCondition(m_videoData.cond);


    SyncAudio();

    AVFrame* frame = m_videoData.frameQueue.GetBeforePts(static_cast<int64_t>(GetSyncClock() / av_q2d(m_videoData.time_base)));
    if (frame != nullptr) {
        m_videoData.clock = frame->pts * av_q2d(m_videoData.time_base);
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


void Video::SyncAudio() {

    

    // while (SDL_GetAudioStreamQueued(m_audioData.m_audioStream) < 5000) {
    //     AVFrame* frame = m_audioData.frameQueue.Get();
    //     if (frame) {
    //         m_audioData.clock = frame->pts * av_q2d(m_audioData.time_base);
    //         if (!SDL_PutAudioStreamData(m_audioData.m_audioStream, frame->data[0], frame->linesize[0])) {
    //             SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to put audio stream data: %s", SDL_GetError());
    //         }
    //         av_frame_free(&frame);
    //     }
    // }

}


