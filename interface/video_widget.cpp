#include "video_widget.h"
#include <iostream>

VideoWidget::VideoWidget(double x, double y, double w, double h, int videoW, int videoH)
    : WidgetClass(x, y, w, h), video_width(videoW), video_height(videoH) {
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, video_width, video_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

VideoWidget::~VideoWidget() {
    glDeleteTextures(1, &texture_id);
}

void VideoWidget::updateFrame(const uint8_t* rgb_data) {
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video_width, video_height, GL_RGB, GL_UNSIGNED_BYTE, rgb_data);
}

void VideoWidget::render() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glBegin(GL_QUADS);
    glTexCoord2f(0.f, 0.f); glVertex2f(xpos, ypos);
    glTexCoord2f(1.f, 0.f); glVertex2f(xpos + width, ypos);
    glTexCoord2f(1.f, 1.f); glVertex2f(xpos + width, ypos + height);
    glTexCoord2f(0.f, 1.f); glVertex2f(xpos, ypos + height);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}
