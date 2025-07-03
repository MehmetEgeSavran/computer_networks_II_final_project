#include "video_decoder.h"
#include "video_encoder.h"
#include "video_redecoder.h"
#include "ui.h"
#include "video_widget.h"

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <iostream>
#include <vector>
#include <thread>

// Cross-platform sockets
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// Helper to send all data
bool sendAll(int sock, const uint8_t* data, int len) {
    int sent = 0;
    while (sent < len) {
#ifdef _WIN32
        int s = send(sock, (const char*)data + sent, len - sent, 0);
#else
        int s = send(sock, data + sent, len - sent, 0);
#endif
        if (s <= 0) return false;
        sent += s;
    }
    return true;
}

// Helper to receive all data
bool recvAll(int sock, uint8_t* data, int len) {
    int recvd = 0;
    while (recvd < len) {
#ifdef _WIN32
        int r = recv(sock, (char*)data + recvd, len - recvd, 0);
#else
        int r = recv(sock, data + recvd, len - recvd, 0);
#endif
        if (r <= 0) return false;
        recvd += r;
    }
    return true;
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    avformat_network_init();

    std::cout << "Server (s) or Client (c)? ";
    char mode;
    std::cin >> mode;

    const int PORT = 5000;
    int sock = -1;
    int client_sock = -1;

    if (mode == 's') {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(PORT);

        bind(sock, (sockaddr*)&addr, sizeof(addr));
        listen(sock, 1);

        std::cout << "Waiting for connection..." << std::endl;
        client_sock = accept(sock, nullptr, nullptr);
        std::cout << "Client connected!" << std::endl;
    } else {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        std::cout << "Connecting..." << std::endl;
        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Failed to connect." << std::endl;
            return -1;
        }
        std::cout << "Connected!" << std::endl;
    }

    if (mode == 's') {
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

        // Signal end
        uint32_t zero = 0;
        sendAll(client_sock, (uint8_t*)&zero, 4);

        av_frame_free(&frame);
        decoder.close();
        encoder.close();
        std::cout << "Server done." << std::endl;

#ifdef _WIN32
        closesocket(sock);
        closesocket(client_sock);
#else
        close(sock);
        close(client_sock);
#endif
        return 0;
    } else {
        // Client: initialize window and decoder
        WindowClass custom_window;
        if (custom_window.getErrorStatus()) return -1;

        WidgetManager widget_manager;
        MouseClass custom_mouse(custom_window.getWindow(), &widget_manager);

        VideoReDecoder redecoder;
        // Receive extradata (could be optional here—skipped for simplicity)

        redecoder.init(nullptr, 0);

        SwsContext* sws_ctx = nullptr;

        AVFrame* decoded = av_frame_alloc();
        uint8_t* rgb_buffer = nullptr;
        VideoWidget* video_widget = nullptr;

        int frame_count = 0;

        bool stop = false;
        while (!glfwWindowShouldClose(custom_window.getWindow()) && !stop) {
            uint32_t size_net;
            if (!recvAll(sock, (uint8_t*)&size_net, 4)) break;
            uint32_t size = ntohl(size_net);
            if (size == 0) break;

            std::vector<uint8_t> packet(size);
            if (!recvAll(sock, packet.data(), size)) break;

            if (redecoder.decodePacket(packet, decoded)) {
                if (!sws_ctx) {
                    sws_ctx = sws_getContext(
                        decoded->width, decoded->height, (AVPixelFormat)decoded->format,
                        decoded->width, decoded->height, AV_PIX_FMT_RGB24,
                        SWS_BILINEAR, nullptr, nullptr, nullptr);
                    rgb_buffer = new uint8_t[decoded->width * decoded->height * 3];

                    float w = static_cast<float>(decoded->width);
                    float h = static_cast<float>(decoded->height);
                    video_widget = new VideoWidget(0,0,w,h, (int)w,(int)h);
                    widget_manager.addNewWidget(video_widget);
                }

                uint8_t* dest[4] = { rgb_buffer, nullptr, nullptr, nullptr };
                int linesize[4] = { decoded->width * 3, 0,0,0 };
                sws_scale(
                    sws_ctx, decoded->data, decoded->linesize,
                    0, decoded->height, dest, linesize);
                video_widget->updateFrame(rgb_buffer);

                // Draw
                glViewport(0,0, (int)(1920*0.85), (int)(1080*0.85));
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(0, (int)(1920*0.85), (int)(1080*0.85), 0, -1,1);
                glMatrixMode(GL_MODELVIEW);

                glClearColor(0.2f,0.3f,0.3f,1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                glLoadIdentity();

                widget_manager.renderAll();

                glfwSwapBuffers(custom_window.getWindow());
                glfwPollEvents();

                av_frame_unref(decoded);
                frame_count++;
            }
        }

        if (sws_ctx) sws_freeContext(sws_ctx);
        if (rgb_buffer) delete[] rgb_buffer;
        if (decoded) av_frame_free(&decoded);
        if (video_widget) delete video_widget;

        redecoder.close();
        std::cout << "Client done." << std::endl;
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return 0;
    }
}
