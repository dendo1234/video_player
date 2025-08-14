#include <memory>
#include <video/Deque.hpp>
#include <SDL3/SDL.h>
#include <video/Deleters.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

struct VideoData {
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> m_videoCodecContext;
    std::unique_ptr<SwsContext, SwsContextDeleter> m_swsContext;
    Deque<AVPacket> packetQueue;
    Deque<AVFrame> frameQueue;
    SDL_Mutex* mutex;
    SDL_Condition* cond;
    AVRational time_base;
    double clock;
    std::unique_ptr<SDL_Texture, SDL_TextureDeleter> texture;
};