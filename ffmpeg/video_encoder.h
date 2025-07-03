#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include <vector>

class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();

    bool init(int width, int height, AVRational time_base);
    bool encodeFrame(AVFrame* frame, std::vector<uint8_t>& out_packet);
    bool setCRF(int crf_value);
    const uint8_t* getExtradata() const;
    int getExtradataSize() const;
    void close();

private:
    AVCodecContext* enc_ctx = nullptr;
};
