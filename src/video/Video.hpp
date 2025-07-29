#include <SDL3/SDL.h>
#include <memory>
#include <queue>
#include <concepts>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ptr) {
        avformat_close_input(&ptr);
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* ptr) {
        sws_freeContext(ptr);
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ptr) {
        avcodec_free_context(&ptr);
    }
};

struct SwrContextDeleter {
    void operator()(SwrContext* ptr) {
        swr_free(&ptr);
    }
};

struct AVPacketDeleter {
    void operator()(AVPacket* ptr) {
        av_packet_free(&ptr);
    }
};

struct SDL_TextureDeleter {
    void operator()(SDL_Texture* ptr) {
        SDL_DestroyTexture(ptr);
    }
};

template <typename T>
concept HasPts = requires(T t) {
    t.pts;
};

template<HasPts T>
class Deque {
private:
    std::deque<T*> deque;
    SDL_Mutex* mutex;
    SDL_Condition* readCond;
    SDL_Condition* writeCond;
    bool flushed{false};
public:
    Deque();
    ~Deque();

    void Push(T* data);
    void PushFront(T* data);
    T* Get();
    size_t Size();
    T* GetBeforePts(int64_t pts);
};

struct AudioData {
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> codecContext;
    std::unique_ptr<SwrContext, SwrContextDeleter> swrContext;
    Deque<AVPacket> packetQueue;
    Deque<AVFrame> frameQueue;
    AVRational time_base;
    SDL_AudioSpec audioSpec;
};

struct VideoData {
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> m_videoCodecContext;
    std::unique_ptr<SwsContext, SwsContextDeleter> m_swsContext;
    Deque<AVPacket> packetQueue;
    Deque<AVFrame> frameQueue;
    SDL_Mutex* mutex;
    SDL_Condition* cond;
    AVRational time_base;
    std::unique_ptr<SDL_Texture, SDL_TextureDeleter> texture;
};

void audio_callback(void *userdata, Uint8 *stream, int len);

class Video {
private:
    SDL_AudioDeviceID m_audioDevideID;

    int GetFormatContext(const char* filename);
    void InitializeThreads();

    SDL_Renderer* renderer;

    SDL_Thread* m_packageReader;
    SDL_Thread* m_videoDecoder;
    SDL_Thread* m_audioDecoder;


public:

    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> m_formatContext;
    int videoStreamIndex = -1;
    int audioStreamIndex = -1;

    AudioData m_audioData;
    VideoData m_videoData;

    bool m_videoDone;
    double m_timestamp;
    long long int m_currentAudioPts;
    long long int m_startTick{-0};


    Video(const char* filename, SDL_Renderer* renderer);
    ~Video();

    void Render() const;
    void Update(float dt);
    void Start();

    bool Is(const std::string& type) const;

};


bool inline Video::Is(const std::string& type) const {
    return type == "Video";
}



/* 
#include <SDL.h>


void RenderVideo(SDL_Renderer* renderer, const char* filename);


 */


