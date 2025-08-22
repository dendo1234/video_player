#include <video/MediaFile.hpp>

using namespace std;


MediaFile::MediaFile(const char* filename) {
    OpenFile(filename);
    FindStreams();
}

void MediaFile::OpenFile(const char* filename) {
    int lastError;

    AVFormatContext* avFormatContextRaw = nullptr;

    lastError = avformat_open_input(&avFormatContextRaw, filename, nullptr, nullptr);
    if(lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error opening video file: %s", buffer);
    }
    context = unique_ptr<AVFormatContext, AVFormatContextDeleter>(avFormatContextRaw);

    lastError = avformat_find_stream_info(context.get(), nullptr);
    if(lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error reading video stream: %s", buffer);
    }

    #ifndef NDEBUG
    av_dump_format(context.get(), 0, filename, 0);
    #endif
}

void MediaFile::FindStreams() {
    for(unsigned i = 0; i < context->nb_streams; i++) {
        if(context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (videoStreamIndex != -1) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "There is more than one video stream in file? Thats undefined!");
            }
            videoStreamIndex = i;
            continue;
        }

        if(context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndices.emplace_back(i);
            continue;
        }
    }
}

void MediaFile::Seek(double timestamp, int flags) {
    (void)flags;
    int64_t convertedTimestamp = static_cast<int64_t>(timestamp*AV_TIME_BASE);
    int64_t minTs = convertedTimestamp - 0.01*AV_TIME_BASE;
    int64_t maxTs = convertedTimestamp + 0.01*AV_TIME_BASE;

    int lastError = avformat_seek_file(context.get(), -1, minTs, convertedTimestamp, maxTs, 0);
    if(lastError < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, lastError);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Error seeking frame: %s", buffer);
    }
}