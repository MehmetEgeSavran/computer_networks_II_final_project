#include "video_server.h"
#include "video_decoder.h"
#include "video_encoder.h"
#include "network_utils.h"
#include <iostream>
#include <thread>
#include <chrono>

extern "C" {
#include <libavutil/imgutils.h>
}

int runVideoServer(int client_sock) {
    const std::string video_path = "D:\\visual_studio_codes\\computer_networks_final_project\\kitty.mp4";

    VideoDecoder decoder;
    if (!decoder.open(video_path)) {
        std::cerr << "Unable to open video.\n";
        return -1;
    }

    VideoEncoder encoder;
    if (!encoder.init(decoder.getWidth(), decoder.getHeight(), decoder.getTimeBase())) {
        std::cerr << "Unable to init encoder.\n";
        return -1;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Failed to allocate frame.\n";
        return -1;
    }

    bool paused = true;

    std::cout << "Server ready. Sending first frame...\n";

    if (decoder.readFrame(frame)) {
        std::vector<uint8_t> packet_data;
        if (encoder.encodeFrame(frame, packet_data)) {
            uint32_t size = htonl(static_cast<uint32_t>(packet_data.size()));
            if (!sendAll(client_sock, reinterpret_cast<uint8_t*>(&size), 4) ||
                !sendAll(client_sock, packet_data.data(), static_cast<int>(packet_data.size()))) {
                std::cerr << "Error sending first frame.\n";
                av_frame_free(&frame);
                decoder.close();
                encoder.close();
                return -1;
            }
        }
        av_frame_unref(frame);
    } else {
        std::cerr << "Failed to read first frame.\n";
        av_frame_free(&frame);
        decoder.close();
        encoder.close();
        return -1;
    }

    std::cout << "First frame sent. Waiting for PLAY command...\n";

    // Main loop
    while (true) {
        // Check for incoming control commands
#ifdef _WIN32
        u_long avail = 0;
        ioctlsocket(client_sock, FIONREAD, &avail);
        if (avail >= 1) {
#else
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(client_sock, &read_fds);
        struct timeval tv = {0, 0};
        int ret = select(client_sock + 1, &read_fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(client_sock, &read_fds)) {
#endif
            char cmd;
            int r = recv(client_sock, &cmd, 1, 0);
            if (r <= 0) {
                std::cerr << "Client disconnected while reading control command.\n";
                break;
            }
            if (cmd == 'P') {
                paused = false;
                std::cout << "Playback resumed.\n";
            } else if (cmd == 'S') {
                paused = true;
                std::cout << "Playback paused.\n";
            }
        }

        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        // Read next video frame
        if (!decoder.readFrame(frame)) {
            // End of video, restart
            decoder.close();
            std::cout << "Reached end of video, restarting...\n";
            if (!decoder.open(video_path)) {
                std::cerr << "Failed to reopen video.\n";
                break;
            }
            continue;
        }

        // Encode and send the frame
        std::vector<uint8_t> packet_data;
        if (encoder.encodeFrame(frame, packet_data)) {
            uint32_t size = htonl(static_cast<uint32_t>(packet_data.size()));
            if (!sendAll(client_sock, reinterpret_cast<uint8_t*>(&size), 4) ||
                !sendAll(client_sock, packet_data.data(), static_cast<int>(packet_data.size()))) {
                std::cerr << "Send error or client disconnected.\n";
                break;
            }
        }
        av_frame_unref(frame);
    }

    // Send end-of-stream marker (size = 0)
    uint32_t zero = 0;
    sendAll(client_sock, reinterpret_cast<uint8_t*>(&zero), 4);

    // Cleanup
    av_frame_free(&frame);
    decoder.close();
    encoder.close();
    std::cout << "Server stopped.\n";
    return 0;
}
