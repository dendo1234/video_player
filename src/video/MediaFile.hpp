#pragma once
#include <memory>
#include <vector>
#include <video/Deleters.hpp>

extern "C" {
#include "libavformat/avformat.h"
}


class MediaFile {
private:
    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> context;

    int videoStreamIndex{-1};
    std::vector<int> audioStreamIndices;

    SDL_Thread* packetReader;

    void OpenFile(const char* filename);
    void FindStreams();

public:
    MediaFile(const char* filename);

    int GetVideoStreamIndex() const;
    std::vector<int> GetAudioStreamIndices() const;

    AVFormatContext* GetFormatContext() const;
    const AVCodecParameters* GetCodecParameters(int streamIndex) const;
    AVRational GetTimeBase(int streamIndex) const;

    void Seek(double timestamp, int flags);

};

inline int MediaFile::GetVideoStreamIndex() const {
    return videoStreamIndex;
}

inline std::vector<int> MediaFile::GetAudioStreamIndices() const {
    return audioStreamIndices;
}

inline AVFormatContext* MediaFile::GetFormatContext() const {
    return context.get();
}

inline const AVCodecParameters* MediaFile::GetCodecParameters(int streamIndex) const {
    return context->streams[streamIndex]->codecpar;
}

inline AVRational MediaFile::GetTimeBase(int streamIndex) const {
    return context->streams[streamIndex]->time_base;
}