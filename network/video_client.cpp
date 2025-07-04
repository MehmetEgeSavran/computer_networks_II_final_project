#include "video_client.h"
#include "video_redecoder.h"
#include "video_widget.h"
#include "network_utils.h"
#include "ui.h"
#include <iostream>
#include <vector>

extern "C" {
#include <libswscale/swscale.h>
}

int runVideoClient(int sock) {
    WindowClass custom_window;
    if (custom_window.getErrorStatus()) return -1;

    WidgetManager widget_manager;
    MouseClass custom_mouse(custom_window.getWindow(), &widget_manager);

    ButtonWidget myButton((int)(1080*0.65), (int)(1920*0.45), 80);
    widget_manager.addNewWidget(&myButton);

    VideoReDecoder redecoder;
    redecoder.init(nullptr, 0);

    SwsContext* sws_ctx = nullptr;
    AVFrame* decoded = av_frame_alloc();
    uint8_t* rgb_buffer = nullptr;
    VideoWidget* video_widget = nullptr;

    bool stop = false;
    int frame_count = 0;

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

                float w = (float)decoded->width;
                float h = (float)decoded->height;
                video_widget = new VideoWidget(0, 0, w, h, (int)w, (int)h);
            }

            uint8_t* dest[4] = { rgb_buffer, nullptr, nullptr, nullptr };
            int linesize[4] = { decoded->width * 3, 0, 0, 0 };
            sws_scale(
                sws_ctx, decoded->data, decoded->linesize, 0, decoded->height,
                dest, linesize);
            video_widget->updateFrame(rgb_buffer);

            glViewport(0, 0, (int)(1920 * 0.85), (int)(1080 * 0.85));
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, (int)(1920 * 0.85), (int)(1080 * 0.85), 0, -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glEnable(GL_TEXTURE_2D);
            video_widget->render();
            glDisable(GL_TEXTURE_2D);

            glColor3f(1.0f, 1.0f, 1.0f);
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

    std::cout << "Client done.\n";
    return 0;
}
