#include "video_encoder.h"
#include <iostream>

VideoEncoder::VideoEncoder() {}

VideoEncoder::~VideoEncoder() {
    close();
}

bool VideoEncoder::init(int width, int height, AVRational time_base) {
    
    const AVCodec* encoder = avcodec_find_encoder_by_name("libx265");
    if (!encoder) {
        std::cerr << "HEVC encoder not found.\n";
        return false;
    }

    enc_ctx = avcodec_alloc_context3(encoder);
    enc_ctx->width = width;
    enc_ctx->height = height;
    enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    enc_ctx->bit_rate = 400000;
    enc_ctx->time_base = time_base;

    if (avcodec_open2(enc_ctx, encoder, nullptr) < 0) {
        std::cerr << "Failed to open encoder.\n";
        return false;
    }

    return true;
}

bool VideoEncoder::encodeFrame(AVFrame* frame, std::vector<uint8_t>& out_packet) {
    if (!enc_ctx) return false;

    if (avcodec_send_frame(enc_ctx, frame) < 0) {
        std::cerr << "Failed to send frame to encoder.\n";
        return false;
    }

    AVPacket* pkt = av_packet_alloc();
    int ret = avcodec_receive_packet(enc_ctx, pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        av_packet_free(&pkt);
        return false;
    }
    if (ret < 0) {
        std::cerr << "Error encoding frame.\n";
        av_packet_free(&pkt);
        return false;
    }

    out_packet.assign(pkt->data, pkt->data + pkt->size);
    av_packet_free(&pkt);
    return true;
}

const uint8_t* VideoEncoder::getExtradata() const {
    return enc_ctx ? enc_ctx->extradata : nullptr;
}

int VideoEncoder::getExtradataSize() const {
    return enc_ctx ? enc_ctx->extradata_size : 0;
}

void VideoEncoder::close() {
    if (enc_ctx) avcodec_free_context(&enc_ctx);
}
