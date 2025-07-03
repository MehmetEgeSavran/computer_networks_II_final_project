#include "video_redecoder.h"
#include <iostream>

VideoReDecoder::VideoReDecoder() {}

VideoReDecoder::~VideoReDecoder() {
    close();
}

bool VideoReDecoder::init(const uint8_t* extradata, int extradata_size) {
    const AVCodec* decoder = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    if (!decoder) {
        std::cerr << "HEVC decoder not found.\n";
        return false;
    }

    dec_ctx = avcodec_alloc_context3(decoder);

    if (extradata && extradata_size > 0) {
        dec_ctx->extradata = (uint8_t*)av_mallocz(extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        memcpy(dec_ctx->extradata, extradata, extradata_size);
        dec_ctx->extradata_size = extradata_size;
    }

    if (avcodec_open2(dec_ctx, decoder, nullptr) < 0) {
        std::cerr << "Failed to open decoder.\n";
        return false;
    }

    return true;
}

bool VideoReDecoder::decodePacket(const std::vector<uint8_t>& packet, AVFrame* out_frame) {
    if (!dec_ctx) return false;

    AVPacket* pkt = av_packet_alloc();
    av_new_packet(pkt, (int)packet.size());
    memcpy(pkt->data, packet.data(), packet.size());

    if (avcodec_send_packet(dec_ctx, pkt) < 0) {
        av_packet_free(&pkt);
        return false;
    }

    int ret = avcodec_receive_frame(dec_ctx, out_frame);
    av_packet_free(&pkt);

    return (ret == 0);
}

void VideoReDecoder::close() {
    if (dec_ctx) avcodec_free_context(&dec_ctx);
}
