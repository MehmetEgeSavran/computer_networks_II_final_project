#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
}

#include <vector>

class VideoReDecoder {
public:
    VideoReDecoder();
    ~VideoReDecoder();

    bool init(const uint8_t* extradata, int extradata_size);
    bool decodePacket(const std::vector<uint8_t>& packet, AVFrame* out_frame);
    void close();

private:
    AVCodecContext* dec_ctx = nullptr;
};
