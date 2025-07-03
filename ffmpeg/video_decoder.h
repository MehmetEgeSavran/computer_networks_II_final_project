#pragma once
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <string>

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

    bool open(const std::string& path);
    bool readFrame(AVFrame* out_frame);
    void close();

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    AVRational getTimeBase() const { return time_base; }

private:
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* dec_ctx = nullptr;
    int video_stream_idx = -1;
    AVPacket* packet = nullptr;

    int width = 0;
    int height = 0;
    AVRational time_base;
};
