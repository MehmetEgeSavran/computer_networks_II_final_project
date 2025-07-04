#include "video_client.h"
#include "video_redecoder.h"
#include "video_widget.h"
#include "network_utils.h"
#include "ui.h"
#include <iostream>
#include <vector>
#include <thread>

extern "C" {
#include <libswscale/swscale.h>
}

int runVideoClient(int sock) {
    WindowClass custom_window;
    if (custom_window.getErrorStatus())
        return -1;

    WidgetManager widget_manager;
    MouseClass custom_mouse(custom_window.getWindow(), &widget_manager);

    ButtonWidget play_button(
        static_cast<int>(1920 * 0.35),
        static_cast<int>(1080 * 0.75),
        60,
        ButtonIconType::Pause,
        true,
        sock
    );
    widget_manager.addNewWidget(&play_button);

    TeardownButtonWidget* teardown_button = new TeardownButtonWidget(
        static_cast<int>(1920 * 0.45),
        static_cast<int>(1080 * 0.75),
        60,
        sock
    );
    widget_manager.addNewWidget(teardown_button);

    VideoReDecoder redecoder;
    redecoder.init(nullptr, 0);

    SwsContext* sws_ctx = nullptr;
    AVFrame* decoded = av_frame_alloc();
    uint8_t* rgb_buffer = nullptr;
    VideoWidget* video_widget = nullptr;

    bool stop = false;

    std::cout << "Client started. Waiting for frames...\n";

    while (!glfwWindowShouldClose(custom_window.getWindow()) && !stop) {
#ifdef _WIN32
        u_long avail = 0;
        ioctlsocket(sock, FIONREAD, &avail);
        bool hasData = avail >= sizeof(uint32_t);
#else
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        struct timeval tv = {0, 0};
        int ret = select(sock + 1, &read_fds, nullptr, nullptr, &tv);
        bool hasData = (ret > 0 && FD_ISSET(sock, &read_fds));
#endif

        if (hasData) {
            uint32_t size_net;
            if (!recvAll(sock, reinterpret_cast<uint8_t*>(&size_net), 4))
                break;
            uint32_t size = ntohl(size_net);
            if (size == 0)
                break;

            std::vector<uint8_t> packet(size);
            if (!recvAll(sock, packet.data(), size))
                break;

            if (redecoder.decodePacket(packet, decoded)) {
                if (!sws_ctx) {
                    sws_ctx = sws_getContext(
                        decoded->width, decoded->height, (AVPixelFormat)decoded->format,
                        decoded->width, decoded->height, AV_PIX_FMT_RGB24,
                        SWS_BILINEAR, nullptr, nullptr, nullptr);

                    rgb_buffer = new uint8_t[decoded->width * decoded->height * 3];

                    float videoW = static_cast<float>(decoded->width);
                    float videoH = static_cast<float>(decoded->height);

                    float windowW = static_cast<float>(1920 * 0.85f);
                    float windowH = static_cast<float>(1080 * 0.85f);

                    float xCentered = (windowW - videoW) * 0.5f;
                    float yCentered = (windowH - videoH) * 0.5f;

                    video_widget = new VideoWidget(
                        xCentered,
                        yCentered,
                        videoW,
                        videoH,
                        static_cast<int>(videoW),
                        static_cast<int>(videoH)
                    );
                }

                uint8_t* dest[4] = { rgb_buffer, nullptr, nullptr, nullptr };
                int linesize[4] = { decoded->width * 3, 0, 0, 0 };
                sws_scale(
                    sws_ctx,
                    decoded->data,
                    decoded->linesize,
                    0,
                    decoded->height,
                    dest,
                    linesize
                );

                video_widget->updateFrame(rgb_buffer);
                av_frame_unref(decoded);
            }
        }

        glViewport(0, 0, static_cast<int>(1920 * 0.85), static_cast<int>(1080 * 0.85));
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, static_cast<int>(1920 * 0.85), static_cast<int>(1080 * 0.85), 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (video_widget) {
            glEnable(GL_TEXTURE_2D);
            video_widget->render();
            glDisable(GL_TEXTURE_2D);
        }

        widget_manager.renderAll();
        glfwSwapBuffers(custom_window.getWindow());
        glfwPollEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    if (sws_ctx) sws_freeContext(sws_ctx);
    if (rgb_buffer) delete[] rgb_buffer;
    if (decoded) av_frame_free(&decoded);
    if (video_widget) delete video_widget;
    redecoder.close();

    std::cout << "Client done.\n";
    return 0;
}
