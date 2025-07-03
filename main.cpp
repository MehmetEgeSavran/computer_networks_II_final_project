#include "video_decoder.h"
#include "video_encoder.h"
#include "video_redecoder.h"
extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
}

#include <iostream>
#include <vector>

// Screen size for window
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

int main() {
    std::cout << "Starting..." << std::endl;
    avformat_network_init();

    VideoDecoder decoder;
    if (!decoder.open("D:\\visual_studio_codes\\computer_networks_final_project\\kitty.mp4")) {
        std::cout << "Unable to open the file." << std::endl;
        return -1;
    }

    VideoEncoder encoder;
    if (!encoder.init(decoder.getWidth(), decoder.getHeight(), decoder.getTimeBase())) {
        std::cout << "Unable to initialize encoder." << std::endl;
        return -1;
    }

    VideoReDecoder redecoder;
    if (!redecoder.init(encoder.getExtradata(), encoder.getExtradataSize())) {
        std::cout << "Unable to initialize re-decoder." << std::endl;
        return -1;
    }

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }
    GLFWwindow* window = glfwCreateWindow(decoder.getWidth(), decoder.getHeight(), "Video Playback", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW." << std::endl;
        return -1;
    }

    // Set up orthographic projection matching video size
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, decoder.getWidth(), decoder.getHeight(), 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    // Create OpenGL texture
    GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, decoder.getWidth(), decoder.getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create SwsContext for YUV->RGB
    SwsContext* sws_ctx = sws_getContext(
        decoder.getWidth(),
        decoder.getHeight(),
        AV_PIX_FMT_YUV420P,
        decoder.getWidth(),
        decoder.getHeight(),
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        nullptr, nullptr, nullptr);

    AVFrame* frame = av_frame_alloc();
    AVFrame* decoded = av_frame_alloc();
    int frame_count = 0;

    uint8_t* rgb_buffer = new uint8_t[decoder.getWidth() * decoder.getHeight() * 3];

    while (!glfwWindowShouldClose(window) && decoder.readFrame(frame)) {
        std::vector<uint8_t> packet_data;
        if (encoder.encodeFrame(frame, packet_data)) {
            if (redecoder.decodePacket(packet_data, decoded)) {
                // Convert YUV to RGB
                uint8_t* dest[4] = { rgb_buffer, nullptr, nullptr, nullptr };
                int linesize[4] = { decoder.getWidth() * 3, 0, 0, 0 };
                sws_scale(sws_ctx, decoded->data, decoded->linesize, 0, decoded->height, dest, linesize);

                // Upload to OpenGL texture
                glBindTexture(GL_TEXTURE_2D, tex_id);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, decoder.getWidth(), decoder.getHeight(), GL_RGB, GL_UNSIGNED_BYTE, rgb_buffer);

                // Draw
                glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                glLoadIdentity();

                glEnable(GL_TEXTURE_2D);
                glBegin(GL_QUADS);
                glTexCoord2f(0.f, 0.f); glVertex2f(0.f, 0.f);
                glTexCoord2f(1.f, 0.f); glVertex2f(decoder.getWidth(), 0.f);
                glTexCoord2f(1.f, 1.f); glVertex2f(decoder.getWidth(), decoder.getHeight());
                glTexCoord2f(0.f, 1.f); glVertex2f(0.f, decoder.getHeight());
                glEnd();
                glDisable(GL_TEXTURE_2D);

                glfwSwapBuffers(window);
                glfwPollEvents();

                av_frame_unref(decoded);

                std::cout << "Frame #" << frame_count++
                          << " displayed size: "
                          << decoded->width << "x" << decoded->height << std::endl;
            }
        }
        av_frame_unref(frame);
    }

    // Cleanup
    delete[] rgb_buffer;
    av_frame_free(&frame);
    av_frame_free(&decoded);
    if (sws_ctx) sws_freeContext(sws_ctx);
    decoder.close();
    encoder.close();
    redecoder.close();

    glDeleteTextures(1, &tex_id);
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Done." << std::endl;
    return 0;
}
