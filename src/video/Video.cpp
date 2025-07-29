

#include <video/Video.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

using namespace std;

template<HasPts T>
Deque<T>::Deque() {
    mutex = SDL_CreateMutex();
    if (mutex == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error creating mutex: %s", SDL_GetError());
    }
    readCond = SDL_CreateCondition();
    if (readCond == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error creating cond: %s", SDL_GetError());
    }
    writeCond = SDL_CreateCondition();
    if (writeCond == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error creating cond: %s", SDL_GetError());
    }
}

template<HasPts T>
void Deque<T>::Push(T* packet) {
    SDL_LockMutex(mutex);
    if (packet == nullptr) {
        flushed = true;
        SDL_SignalCondition(writeCond);
    } else {
        while (deque.size() > 10 && !flushed) {
            SDL_WaitCondition(writeCond, mutex);
        }
    }
    deque.push_back(packet);
    SDL_SignalCondition(readCond);
    SDL_UnlockMutex(mutex);
    
}

template<HasPts T>
void Deque<T>::PushFront(T* packet) {
    SDL_LockMutex(mutex);
    deque.push_front(packet);
    SDL_SignalCondition(readCond);
    SDL_UnlockMutex(mutex);
}

template<HasPts T>
T* Deque<T>::Get() {
    SDL_LockMutex(mutex);
    while (deque.size() == 0) {
        SDL_WaitCondition(readCond,mutex);
    }
    T* ptr = deque.front();
    deque.pop_front();
    SDL_SignalCondition(writeCond);

    SDL_UnlockMutex(mutex);
    
    return ptr;
}


template<HasPts T>
size_t Deque<T>::Size() {
    return deque.size();
}

template<HasPts T>
Deque<T>::~Deque() {
    SDL_DestroyMutex(mutex);
    SDL_DestroyCondition(readCond);
}

template <HasPts T>
T* Deque<T>::GetBeforePts(int64_t pts) {
    SDL_LockMutex(mutex);
    if (deque.size() == 0) {
        SDL_WaitCondition(readCond,mutex);
    }
    T* oldPtr = nullptr;
    T* ptr = deque.front();

    if (ptr == nullptr) {
        return nullptr;
    }

    while (ptr->pts < pts && deque.size() > 1) {
        if (oldPtr != nullptr) {
            av_frame_free(&oldPtr);
        };
        oldPtr = ptr;
        deque.pop_front();
        ptr = deque.front();
        if (ptr == nullptr) {
            return nullptr;
        }
    } 

    SDL_SignalCondition(writeCond);
    SDL_UnlockMutex(mutex);
    return oldPtr;
}


void AudioCallback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    if (additional_amount <= 0) {
        return;
    }


    Video* video = (Video*)userdata;
    AudioData* data = &video->m_audioData;
    if (video->m_videoDone) {
        memset(stream, 0, total_amount);
        return;
    }

    AVFrame* frame = data->frameQueue.Get();
    if (frame == nullptr) {
        memset(stream, 0, total_amount);
        video->m_videoDone = true;
        av_frame_free(&frame);
        return;
    }
    video->m_currentAudioPts = frame->pts;
    SDL_PutAudioStreamData(stream, frame->data[0], frame->linesize[0]);

    av_frame_free(&frame);
    return;
}

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
    Video* video = (Video*)userdata;
    AudioData* data = &video->m_audioData;
    AVFrame* frame = av_frame_alloc();

    AVPacket* packet;
    bool decoding = true;

    while (true) {
        lastError = avcodec_receive_frame(data->codecContext.get(), frame);

        if (lastError == 0) {
            // recebeu frame
            AVFrame* resultFrame = av_frame_alloc();
            resultFrame->format = AV_SAMPLE_FMT_S16;
            resultFrame->ch_layout = data->codecContext->ch_layout;
            resultFrame->nb_samples = data->codecContext->frame_size;
            av_frame_get_buffer(resultFrame, 0);

            swr_convert(data->swrContext.get(), (uint8_t**)resultFrame->data, resultFrame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
            resultFrame->pts = frame->pts;

            data->frameQueue.Push(resultFrame);

            // av_frame_free(&frame);
            continue;
        }

        else if (lastError == AVERROR_EOF || video->m_videoDone) {
            // acabou os frames
            data->frameQueue.Push(nullptr);
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
                SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error sending package to audio decode: %s", buffer);
                continue;
            }
        }
    }
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
    : renderer{renderer},
      m_videoDone{false}, 
      m_timestamp{0} {
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

    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &m_audioData.audioSpec, AudioCallback, this);

    if (!SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream))) {
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

    SDL_PauseAudioDevice(m_audioDevideID);
     
    // initialize VideoData
    m_videoData.texture = unique_ptr<SDL_Texture, SDL_TextureDeleter>(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, m_videoData.m_videoCodecContext->width, m_videoData.m_videoCodecContext->height));
    m_videoData.mutex = SDL_CreateMutex();
    m_videoData.cond = SDL_CreateCondition();

    InitializeThreads();

    // seta o framerate do jogo para ser o framerate do vídeo
    int framerate = static_cast<int>((double)m_videoData.m_videoCodecContext->framerate.den * 1000 / m_videoData.m_videoCodecContext->framerate.num);
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

void Video::Update(float dt) {
    if (m_videoDone == true) {
        return;
    }

    m_timestamp += dt;
    SDL_SignalCondition(m_videoData.cond);
    int lastError;

    
    AVFrame* frame = m_videoData.frameQueue.GetBeforePts(static_cast<int64_t>((double)m_currentAudioPts * av_q2d(m_audioData.time_base) / av_q2d(m_videoData.time_base)));
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

    // SDL_LockMutex(m_videoData.mutex);
    // if (m_timestamp < m_videoData.pts) {
    //     SDL_UnlockMutex(m_videoData.mutex);
    //     return;
    // }

    bool lastError = SDL_RenderTexture(renderer, m_videoData.texture.get(), nullptr, nullptr);
    if (lastError == false) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error rendering video texture: %s", SDL_GetError());
    }
    // SDL_UnlockMutex(m_videoData.mutex);


}



