#pragma once
extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
}
#include <string>

//video decoder class
//the decoder&encoder and redecoder classes could have been built with a same base class, but this approach seemed less bloated
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
    AVFormatContext* format_context = nullptr;
    AVCodecContext* decoder_context = nullptr;
    int video_stream_idx = -1;
    AVPacket* packet = nullptr;

    int width = 0;
    int height = 0;
    AVRational time_base;
};
