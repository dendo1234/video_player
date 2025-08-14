#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include <queue>
#include <array>
#include <concepts>
#include <string>
#include <video/Deque.hpp>
#include <basic/Clock.hpp>
#include <video/AudioData.hpp>
#include <video/Deleters.hpp>
#include <video/VideoData.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}




class Video {
private:
    SDL_AudioDeviceID m_audioDevideID{0};

    int GetFormatContext(const char* filename);
    void InitializeStreamsIndexes();
    int InitializeDecoders();
    void InitializeAudioStreams();
    int InitializeSwrContexts();
    void InitializeThreads();

    SDL_Renderer* renderer;

    SDL_Thread* m_packageReader;
    SDL_Thread* m_videoDecoder;

    Clock clock;

public:

    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> m_formatContext;
    int videoStreamIndex = -1;
    int numberOfAudioStreams = 0;

    std::vector<AudioData> audios;
    VideoData m_videoData;


    bool m_videoDone{false};
    long long int m_startTick{-0};


    Video(const char* filename, SDL_Renderer* renderer);
    ~Video();

    double GetSyncClock();

    void SyncAudio();

    void Render() const;
    void Update(uint64_t dt);
    void Start();

    bool Is(const std::string& type) const;

};


bool inline Video::Is(const std::string& type) const {
    return type == "Video";
}

