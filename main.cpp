#include "video_decoder.h"
#include "video_encoder.h"
#include "video_redecoder.h"
#include "ui.h"

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
    if (custom_window.getErrorStatus()) {
        return -1;
    }

    WidgetManager widget_manager;
    MouseClass custom_mouse(custom_window.getWindow(), &widget_manager);

    // Create OpenGL texture
    GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        decoder.getWidth(),
        decoder.getHeight(),
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

                glBindTexture(GL_TEXTURE_2D, tex_id);
                glTexSubImage2D(
                    GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    decoder.getWidth(),
                    decoder.getHeight(),
                    GL_RGB,
                    GL_UNSIGNED_BYTE,
                    rgb_buffer);

                // Query window size
                int win_w, win_h;
                glfwGetFramebufferSize(custom_window.getWindow(), &win_w, &win_h);

                // Compute scaling factor
                float scale_x = static_cast<float>(win_w) / decoder.getWidth();
                float scale_y = static_cast<float>(win_h) / decoder.getHeight();
                float scale = std::min(1.0f, std::min(scale_x, scale_y));

                // Compute scaled size
                float draw_w = decoder.getWidth() * scale;
                float draw_h = decoder.getHeight() * scale;

                // Centering offsets
                float offset_x = (win_w - draw_w) * 0.5f;
                float offset_y = (win_h - draw_h) * 0.5f;

                // Viewport and projection
                glViewport(0, 0, win_w, win_h);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(0, win_w, win_h, 0, -1, 1);
                glMatrixMode(GL_MODELVIEW);

                // Clear
                glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                glLoadIdentity();

                // Draw quad
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, tex_id);

                glBegin(GL_QUADS);
                glTexCoord2f(0.f, 0.f); glVertex2f(offset_x, offset_y);
                glTexCoord2f(1.f, 0.f); glVertex2f(offset_x + draw_w, offset_y);
                glTexCoord2f(1.f, 1.f); glVertex2f(offset_x + draw_w, offset_y + draw_h);
                glTexCoord2f(0.f, 1.f); glVertex2f(offset_x, offset_y + draw_h);
                glEnd();

                glDisable(GL_TEXTURE_2D);

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
    glDeleteTextures(1, &tex_id);

    std::cout << "Done." << std::endl;
    return 0;
}
