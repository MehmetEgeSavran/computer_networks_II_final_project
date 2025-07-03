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

int main() {
    std::cout << "Starting..." << std::endl;
    avformat_network_init();

    VideoDecoder decoder;
    if (!decoder.open("D:\\visual_studio_codes\\computer_networks_final_project\\kitty.mp4")) {
        std::cerr << "Unable to open the file." << std::endl;
        return -1;
    }

    VideoEncoder encoder;
    if (!encoder.init(decoder.getWidth(), decoder.getHeight(), decoder.getTimeBase())) {
        std::cerr << "Unable to initialize encoder." << std::endl;
        return -1;
    }

    VideoReDecoder redecoder;
    if (!redecoder.init(encoder.getExtradata(), encoder.getExtradataSize())) {
        std::cerr << "Unable to initialize re-decoder." << std::endl;
        return -1;
    }

    WindowClass custom_window;
    if (custom_window.getErrorStatus()) return -1;

    WidgetManager widget_manager;
    MouseClass custom_mouse(custom_window.getWindow(), &widget_manager);

    SwsContext* sws_ctx = sws_getContext(
        decoder.getWidth(),
        decoder.getHeight(),
        AV_PIX_FMT_YUV420P,
        decoder.getWidth(),
        decoder.getHeight(),
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        nullptr, nullptr, nullptr);
    if (!sws_ctx) {
        std::cerr << "Failed to create sws context." << std::endl;
        return -1;
    }

    AVFrame* frame = av_frame_alloc();
    AVFrame* decoded = av_frame_alloc();
    if (!frame || !decoded) {
        std::cerr << "Failed to allocate frames." << std::endl;
        return -1;
    }

    const int rgb_stride = decoder.getWidth() * 3;
    const size_t rgb_buf_size = rgb_stride * decoder.getHeight();
    uint8_t* rgb_buffer = new(std::nothrow) uint8_t[rgb_buf_size];
    if (!rgb_buffer) {
        std::cerr << "Failed to allocate RGB buffer." << std::endl;
        return -1;
    }

    // Create the VideoWidget centered and scaled
    int win_w = 1920 * 0.85;
    int win_h = 1080 * 0.85;
    float scale_x = float(win_w) / decoder.getWidth();
    float scale_y = float(win_h) / decoder.getHeight();
    float scale = std::min(1.0f, std::min(scale_x, scale_y));
    float draw_w = decoder.getWidth() * scale;
    float draw_h = decoder.getHeight() * scale;
    float offset_x = (win_w - draw_w) * 0.5f;
    float offset_y = (win_h - draw_h) * 0.5f;

    VideoWidget video_widget(offset_x, offset_y, draw_w, draw_h, decoder.getWidth(), decoder.getHeight());
    widget_manager.addNewWidget(&video_widget);

    int frame_count = 0;

    while (!glfwWindowShouldClose(custom_window.getWindow()) && decoder.readFrame(frame)) {
        std::vector<uint8_t> packet_data;
        if (encoder.encodeFrame(frame, packet_data)) {
            if (redecoder.decodePacket(packet_data, decoded)) {
                uint8_t* dest[4] = { rgb_buffer, nullptr, nullptr, nullptr };
                int linesize[4] = { rgb_stride, 0, 0, 0 };

                sws_scale(
                    sws_ctx,
                    decoded->data,
                    decoded->linesize,
                    0,
                    decoded->height,
                    dest,
                    linesize);

                video_widget.updateFrame(rgb_buffer);

                // Clear
                glViewport(0, 0, win_w, win_h);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(0, win_w, win_h, 0, -1, 1);
                glMatrixMode(GL_MODELVIEW);
                glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                glLoadIdentity();

                widget_manager.renderAll();

                glfwSwapBuffers(custom_window.getWindow());
                glfwPollEvents();

                av_frame_unref(decoded);

                std::cout << "Frame #" << frame_count++
                          << " displayed size: "
                          << decoded->width << "x" << decoded->height << std::endl;
            }
        }
        av_frame_unref(frame);
    }

    delete[] rgb_buffer;
    av_frame_free(&frame);
    av_frame_free(&decoded);
    sws_freeContext(sws_ctx);
    decoder.close();
    encoder.close();
    redecoder.close();

    std::cout << "Done." << std::endl;
    return 0;
}
