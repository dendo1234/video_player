#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include <queue>
#include <array>
#include <concepts>
#include <string>
#include <video/Deque.hpp>
#include <basic/Clock.hpp>
#include <video/Deleters.hpp>
#include <video/MediaFile.hpp>
#include <stream/VideoStream.hpp>
#include <stream/AudioStream.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}




class Video {
public:
    bool m_videoDone{false};
private:
    MediaFile mediaFile;
    VideoStream videoStream;
    SDL_AudioDeviceID m_audioDevideID{0};
    std::vector<AudioStream> audioStreams;

    SDL_Renderer* renderer;
    std::unique_ptr<SDL_Texture, SDL_TextureDeleter> texture;

    std::unique_ptr<SDL_Thread, SDL_ThreadDeleter> packageReader;

    Clock clock;

    unique_ptr<AVCodecContext, AVCodecContextDeleter> InitializeDecoder(int streamIndex);
    VideoStream InitializeVideoStream();
    std::vector<AudioStream> InitializeAudioStreams();
    void InitializeThreads();

    void FlushStreams();

    static int PacketReaderThread(void* userdata);


public:
    long long int m_startTick{-0};


    Video(const char* filename, SDL_Renderer* renderer);
    ~Video();

    double GetSyncClock();
    SDL_AudioDeviceID GetAudioDeviceID();

    void Render() const;
    void Update(uint64_t dt);
    void Start();

    bool Is(const std::string& type) const;

};


bool inline Video::Is(const std::string& type) const {
    return type == "Video";
}

inline SDL_AudioDeviceID Video::GetAudioDeviceID() {
    return m_audioDevideID;
}
