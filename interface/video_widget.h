#pragma once
#include "ui.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class VideoWidget : public WidgetClass {
public:
    VideoWidget(double x, double y, double w, double h, int videoW, int videoH);
    ~VideoWidget();
    void render() override;
    void updateFrame(const uint8_t* rgb_data);

private:
    GLuint texture_id;
    int video_width;
    int video_height;
};
