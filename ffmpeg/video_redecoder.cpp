#include "video_redecoder.h"
#include <iostream>

VideoReDecoder::VideoReDecoder() {}
VideoReDecoder::~VideoReDecoder() { close(); }

bool VideoReDecoder::init(const uint8_t* extradata, int extradata_size) {
    const AVCodec* decoder = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    if (!decoder) {
        std::cerr << "HEVC decoder not found.\n";
        return false;
    }
    decoder_context = avcodec_alloc_context3(decoder);
    if (extradata && extradata_size > 0) {
        decoder_context->extradata = (uint8_t*)av_mallocz(extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        memcpy(decoder_context->extradata, extradata, extradata_size);
        decoder_context->extradata_size = extradata_size;
    }
    if (avcodec_open2(decoder_context, decoder, nullptr) < 0) {
        std::cerr << "Failed to open decoder.\n";
        return false;
    }
    return true;
}

bool VideoReDecoder::decodePacket(const std::vector<uint8_t>& packet, AVFrame* out_frame) {
    if (!decoder_context) return false;
    AVPacket* allocated_packet = av_packet_alloc();
    av_new_packet(allocated_packet, (int)packet.size());
    memcpy(allocated_packet->data, packet.data(), packet.size());
    if (avcodec_send_packet(decoder_context, allocated_packet) < 0) {
        av_packet_free(&allocated_packet);
        return false;
    }
    int return_value = avcodec_receive_frame(decoder_context, out_frame);
    av_packet_free(&allocated_packet);
    return (return_value == 0);
}

void VideoReDecoder::close() {
    if (decoder_context) avcodec_free_context(&decoder_context);
}
