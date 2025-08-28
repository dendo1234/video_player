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


    void OpenFile(const char* filename);
    void FindStreams();

public:
    MediaFile(const char* filename);

    int GetVideoStreamIndex() const;
    std::vector<int> GetAudioStreamIndices() const;
    uint32_t GetNumberOfStreams() const;

    AVFormatContext* GetFormatContext() const;
    const AVCodecParameters* GetCodecParameters(int streamIndex) const;
    AVRational GetTimeBase(int streamIndex) const;
    double GetStartTime() const;
    double GetDuration() const;

    // Performas a Seek to the nearest keyframe (I-Frame) to the timestamp
    // Delta value avoids going backwards
    void Seek(double timestamp, double delta, int flags = 0);

};

inline int MediaFile::GetVideoStreamIndex() const {
    return videoStreamIndex;
}

inline std::vector<int> MediaFile::GetAudioStreamIndices() const {
    return audioStreamIndices;
}

inline uint32_t MediaFile::GetNumberOfStreams() const {
    return context->nb_streams;
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

inline double MediaFile::GetStartTime() const {
    return static_cast<double>(context->start_time)/AV_TIME_BASE;
}
inline double MediaFile::GetDuration() const {
    return static_cast<double>(context->duration)/AV_TIME_BASE;
}