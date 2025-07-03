#include "video_decoder.h"
#include <iostream>

VideoDecoder::VideoDecoder() {}

VideoDecoder::~VideoDecoder() {
    close();
}

bool VideoDecoder::open(const std::string& path) {
    if (avformat_open_input(&fmt_ctx, path.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Failed to open input.\n";
        return false;
    }
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        std::cerr << "Failed to find stream info.\n";
        return false;
    }
    video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_idx < 0) {
        std::cerr << "No video stream.\n";
        return false;
    }
    AVStream* in_stream = fmt_ctx->streams[video_stream_idx];
    const AVCodec* decoder = avcodec_find_decoder(in_stream->codecpar->codec_id);
    dec_ctx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(dec_ctx, in_stream->codecpar);
    avcodec_open2(dec_ctx, decoder, nullptr);

    width = dec_ctx->width;
    height = dec_ctx->height;
    time_base = in_stream->time_base;

    packet = av_packet_alloc();

    return true;
}

bool VideoDecoder::readFrame(AVFrame* out_frame) {
    while (true) {
        int ret = av_read_frame(fmt_ctx, packet);
        if (ret < 0) {
            // Try flushing decoder
            ret = avcodec_send_packet(dec_ctx, nullptr);
            if (ret < 0) return false;
        } else {
            if (packet->stream_index != video_stream_idx) {
                av_packet_unref(packet);
                continue;
            }

            if (avcodec_send_packet(dec_ctx, packet) < 0) {
                av_packet_unref(packet);
                continue;
            }
            av_packet_unref(packet);
        }

        // Keep receiving frames until we get one or no more
        while (true) {
            ret = avcodec_receive_frame(dec_ctx, out_frame);
            if (ret == AVERROR(EAGAIN)) {
                // Need to read more packets
                break;
            }
            if (ret == AVERROR_EOF) {
                return false;
            }
            if (ret < 0) {
                std::cerr << "Error during decoding.\n";
                return false;
            }
            // Got a frame
            return true;
        }
    }
}


void VideoDecoder::close() {
    if (packet) av_packet_free(&packet);
    if (dec_ctx) avcodec_free_context(&dec_ctx);
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
}
