#pragma once
#include <SDL3/SDL.h>
#include <thread>
#include <atomic>
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
#include <core/Layer.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}


struct ReadPacketReturn {
    enum Status {
        SUCESS,
        END, // EOF
        FAILURE
    } status;
    int streamIndex{0};
    int64_t pts{0};
};

class Player; // "basic/Player.hpp"

class Video : public Layer {
public:
    bool m_videoDone{false};
private:
    MediaFile mediaFile;
    VideoStream videoStream;
    SDL_AudioDeviceID m_audioDevideID{0};
    std::vector<AudioStream> audioStreams;

    std::unique_ptr<SDL_Texture, SDL_TextureDeleter> texture;

    std::jthread packageReader;

    Clock clock;

    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> InitializeDecoder(int streamIndex);
    VideoStream InitializeVideoStream();
    std::vector<AudioStream> InitializeAudioStreams();
    void InitializeThreads();

    void FlushStreams();

    // Should only be called on the PacketReader Thread
    void Seek(double timestamp, double delta);

    static ReadPacketReturn ReadPacket(Video* video);
    static int PacketReaderThread(void* userdata);


public:
    long long int m_startTick{-0};

    struct SeekInterface {
        std::atomic<bool> seekRequested{false};
        double targetTimestamp;
        // The delta value is used to diferentiate a relative seek from the current timestamp form a absolute seek
        // delta = 0 means absolute seek
        double delta;
    } seekInterface;

    Video(const Layer& layer, const char* filename);
    ~Video();

    int GetVideoWidth();
    int GetVideoHeight();
    double GetSyncClock();
    SDL_AudioDeviceID GetAudioDeviceID();

    void RequestSeek(double targetTimestamp, double delta = 0);
    void TogglePause();

    void OnRender() override;
    void OnUpdate(double dt) override;
    EventResult OnEvent(const Event& event) override;
    void GuiPass();

};

inline SDL_AudioDeviceID Video::GetAudioDeviceID() {
    return m_audioDevideID;
}
