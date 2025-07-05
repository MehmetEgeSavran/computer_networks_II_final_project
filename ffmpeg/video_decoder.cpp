#include "video_decoder.h"
#include <iostream>

//No real need for a constructor and the destructor just cleans up ie frees the allocated memories
VideoDecoder::VideoDecoder() {}
VideoDecoder::~VideoDecoder() { close(); }

bool VideoDecoder::open(const std::string& video_path) {
    if (avformat_open_input(&format_context, video_path.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Failed to open input of path.\n";
        return false;
    }
    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        std::cerr << "Failed to find stream info of the path.\n";
        return false;
    }
    video_stream_idx = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_idx < 0) {
        std::cerr << "No video stream.\n";
        return false;
    }
    AVStream* in_stream = format_context->streams[video_stream_idx];
    const AVCodec* decoder = avcodec_find_decoder(in_stream->codecpar->codec_id);
    decoder_context = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decoder_context, in_stream->codecpar);
    avcodec_open2(decoder_context, decoder, nullptr);

    width = decoder_context->width;
    height = decoder_context->height;
    time_base = in_stream->time_base;

    packet = av_packet_alloc();

    return true;
}

bool VideoDecoder::readFrame(AVFrame* out_frame) {
    while (true) {
        int return_value = av_read_frame(format_context, packet);
        if (return_value < 0) {
            return_value = avcodec_send_packet(decoder_context, nullptr);
            if (return_value < 0) return false;
        } else {
            if (packet->stream_index != video_stream_idx) {
                av_packet_unref(packet);
                continue;
            }

            if (avcodec_send_packet(decoder_context, packet) < 0) {
                av_packet_unref(packet);
                continue;
            }
            av_packet_unref(packet);
        }

        while (true) {
            return_value = avcodec_receive_frame(decoder_context, out_frame);
            if (return_value == AVERROR(EAGAIN)) {
                break;
            }
            if (return_value == AVERROR_EOF) {
                return false;
            }
            if (return_value < 0) {
                std::cerr << "Error during decoding.\n";
                return false;
            }
            return true;
        }
    }
}

void VideoDecoder::close() {
    if (packet) av_packet_free(&packet);
    if (decoder_context) avcodec_free_context(&decoder_context);
    if (format_context) avformat_close_input(&format_context);
}
