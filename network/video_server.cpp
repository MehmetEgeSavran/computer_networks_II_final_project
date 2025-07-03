#include "video_server.h"
#include "video_decoder.h"
#include "video_encoder.h"
#include "network_utils.h"
#include <iostream>
extern "C" {
#include <libavutil/imgutils.h>
}

int runVideoServer(int client_sock) {
    VideoDecoder decoder;
    if (!decoder.open("D:\\visual_studio_codes\\computer_networks_final_project\\kitty.mp4")) {
        std::cerr << "Unable to open video." << std::endl;
        return -1;
    }

    VideoEncoder encoder;
    if (!encoder.init(decoder.getWidth(), decoder.getHeight(), decoder.getTimeBase())) {
        std::cerr << "Unable to init encoder." << std::endl;
        return -1;
    }

    AVFrame* frame = av_frame_alloc();
    while (decoder.readFrame(frame)) {
        std::vector<uint8_t> packet_data;
        if (encoder.encodeFrame(frame, packet_data)) {
            uint32_t size = htonl(static_cast<uint32_t>(packet_data.size()));
            if (!sendAll(client_sock, (uint8_t*)&size, 4) ||
                !sendAll(client_sock, packet_data.data(), (int)packet_data.size())) {
                std::cerr << "Send error." << std::endl;
                break;
            }
        }
        av_frame_unref(frame);
    }

    // Send termination signal
    uint32_t zero = 0;
    sendAll(client_sock, (uint8_t*)&zero, 4);

    av_frame_free(&frame);
    decoder.close();
    encoder.close();
    std::cout << "Server done streaming.\n";
    return 0;
}
