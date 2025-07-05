#include "video_encoder.h"
    #include <iostream>
    extern "C" {
#include <libavutil/opt.h>
}


VideoEncoder::VideoEncoder() : encoder_context(nullptr) {}

VideoEncoder::~VideoEncoder() { close(); }

bool VideoEncoder::init(int width, int height, AVRational time_base) {
    const AVCodec* encoder = avcodec_find_encoder_by_name("libx265");
    if (!encoder) {
        std::cerr << "HEVC encoder not found.\n";
        return false;
    }
    encoder_context = avcodec_alloc_context3(encoder);
    if (!encoder_context) {
        std::cerr << "Failed to allocate encoder context.\n";
        return false;
    }
    encoder_context->width = width;
    encoder_context->height = height;
    encoder_context->pix_fmt = AV_PIX_FMT_YUV420P;
    if (av_opt_set(encoder_context->priv_data, "crf", "23", 0) < 0) {   
        std::cerr << "Failed to set CRF.\n";
        return false;
    }
    encoder_context->time_base = time_base;
    if (avcodec_open2(encoder_context, encoder, nullptr) < 0) {
        std::cerr << "Failed to open encoder.\n";
        return false;
    }
    return true;
}

bool VideoEncoder::encodeFrame(AVFrame* frame, std::vector<uint8_t>& out_packet) {
    if (!encoder_context) return false;
    if (avcodec_send_frame(encoder_context, frame) < 0) {
        std::cerr << "Failed to send frame to encoder.\n";
        return false;
    }
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        std::cerr << "Failed to allocate packet.\n";
        return false;
    }
    int return_value = avcodec_receive_packet(encoder_context, pkt);
    if (return_value == AVERROR(EAGAIN) || return_value == AVERROR_EOF) {
        av_packet_free(&pkt);
        return false;
    }
    if (return_value < 0) {
        std::cerr << "Error encoding frame.\n";
        av_packet_free(&pkt);
        return false;
    }
    out_packet.assign(pkt->data, pkt->data + pkt->size);
    av_packet_free(&pkt);
    return true;
}

bool VideoEncoder::setCRF(int crf_value) {
    if (!encoder_context) {
        std::cerr << "Cannot set CRF: encoder not initialized.\n";
        return false;
    }
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d", crf_value);
    if (av_opt_set(encoder_context->priv_data, "crf", buffer, 0) < 0) {
        std::cerr << "Failed to set CRF to " << crf_value << ".\n";
        return false;
    }
    return true;
}

const uint8_t* VideoEncoder::getExtradata() const { return encoder_context ? encoder_context->extradata : nullptr; }

int VideoEncoder::getExtradataSize() const { return encoder_context ? encoder_context->extradata_size : 0; }

void VideoEncoder::close() {
    if (encoder_context) {
        avcodec_free_context(&encoder_context);
        encoder_context = nullptr;
    }
}
