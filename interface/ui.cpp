#include "ui.h"
#include <iostream>
#include <cmath>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <unistd.h>
#endif

#define PI 3.1415926f

WindowClass::WindowClass() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        WINDOW_CLASS_ERROR = true;
        return;
    }
    //the window resolution here was originally set to 1920:1080 but it is later changed to *0.85 for easier time at debugging
    window = glfwCreateWindow(1920 * 0.85, 1080 * 0.85, "OpenGL Window", nullptr, nullptr); 
    if (!window) {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        WINDOW_CLASS_ERROR = true;
        return;
    }
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW.\n";
        WINDOW_CLASS_ERROR = true;
    }
}

WindowClass::~WindowClass() {
    if (window)
        glfwDestroyWindow(window);
    glfwTerminate();
}

GLFWwindow* WindowClass::getWindow() { return window; }
bool WindowClass::getErrorStatus() { return WINDOW_CLASS_ERROR; }

WidgetClass::WidgetClass(double x, double y, double w, double h)
    : x_position(x), y_position(y), width(w), height(h) {}

WidgetClass::~WidgetClass() {}

void WidgetClass::onMouseEvent(int button, int action, double mouse_x, double mouse_y) {
    if (!contains(mouse_x, mouse_y))
        return;
    if (action == GLFW_PRESS) {
        pressed = true;
    } else if (action == GLFW_RELEASE) {
        pressed = false;
    }
}

bool WidgetClass::contains(double x, double y) {
    return (x >= x_position) && (x <= x_position + width) && (y >= y_position) && (y <= y_position + height);
}

bool WidgetClass::isPressed() const { return pressed; }
std::vector<double> WidgetClass::getPos() { return { x_position, y_position }; }
std::vector<double> WidgetClass::getDim() { return { height, width }; }

void WidgetManager::addNewWidget(WidgetClass* widget) {
    widgets.push_back(widget);
}
void WidgetManager::renderAll() {
    for (auto widget_element : widgets) {
        widget_element->render();
    }
}
void WidgetManager::handleMouseEvent(int button, int action, double x, double y) {
    for (auto widget_element : widgets) {
        widget_element->onMouseEvent(button, action, x, y);
    }
}

MouseClass::MouseClass(GLFWwindow* window, WidgetManager* manager)
    : parent_window(window), widget_manager(manager) {
    glfwSetWindowUserPointer(parent_window, this);
    glfwSetKeyCallback(parent_window, keyCallback);
    glfwSetMouseButtonCallback(parent_window, mousePressCallback);
    glfwSetCursorPosCallback(parent_window, mouseMoveCallback);
}
MouseClass::~MouseClass() {}

void MouseClass::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
    }
}
void MouseClass::mousePressCallback(GLFWwindow* window, int button, int action, int mods) {
    MouseClass* instance = static_cast<MouseClass*>(glfwGetWindowUserPointer(window));
    if (!instance) return;
    if (instance->widget_manager) {
        instance->widget_manager->handleMouseEvent(button, action, instance->xpos, instance->ypos);
    }
}
void MouseClass::mouseMoveCallback(GLFWwindow* window, double x, double y) {
    MouseClass* instance = static_cast<MouseClass*>(glfwGetWindowUserPointer(window));
    if (!instance) return;
    instance->xpos = x;
    instance->ypos = y;
}
ButtonWidget::ButtonWidget(double x, double y, double radius, ButtonIconType icon, bool toggle, int socket_fd)
    : WidgetClass(x - radius, y - radius, radius * 2, radius * 2),
      radius(radius),
      current_icon(icon),
      toggleOnClick(toggle),
      sock_fd(socket_fd) {}

void ButtonWidget::render() {
    if (isPressed())
        glColor3f(1.0f, 0.3f, 0.3f);    //button colors change on action both as a debugging method and as a feature
    else
        glColor3f(0.3f, 0.8f, 0.3f);

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(getPos().at(0) + radius, getPos().at(1) + radius);
    const int segments = 50;
    for (int index = 0; index <= segments; ++index) {
        float angle = index * 2.0f * PI / segments;
        float differential_x = cosf(angle) * radius;
        float differential_y = sinf(angle) * radius;
        glVertex2f(getPos().at(0) + radius + differential_x, getPos().at(1) + radius + differential_y);
    }
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    float corner_x = getPos().at(0) + radius;
    float corner_y = getPos().at(1) + radius;
    if (current_icon == ButtonIconType::Play) {
        float alignment_variable = radius * 0.6f;
        glBegin(GL_TRIANGLES);
        glVertex2f(corner_x - alignment_variable * 0.4f, corner_y - alignment_variable);
        glVertex2f(corner_x - alignment_variable * 0.4f, corner_y + alignment_variable);
        glVertex2f(corner_x + alignment_variable * 0.6f, corner_y);
        glEnd();
    } else if (current_icon == ButtonIconType::Pause) {
        float pause_icon_width = radius * 0.2f;
        float pause_icon_height = radius * 0.7f;
        glBegin(GL_QUADS);
        
        glVertex2f(corner_x - pause_icon_width - pause_icon_width, corner_y - pause_icon_height);
        glVertex2f(corner_x - pause_icon_width, corner_y - pause_icon_height);
        glVertex2f(corner_x - pause_icon_width, corner_y + pause_icon_height);
        glVertex2f(corner_x - pause_icon_width - pause_icon_width, corner_y + pause_icon_height);
        
        glVertex2f(corner_x + pause_icon_width, corner_y - pause_icon_height);
        glVertex2f(corner_x + pause_icon_width + pause_icon_width, corner_y - pause_icon_height);
        glVertex2f(corner_x + pause_icon_width + pause_icon_width, corner_y + pause_icon_height);
        glVertex2f(corner_x + pause_icon_width, corner_y + pause_icon_height);
        glEnd();
    }
}

void ButtonWidget::onMouseEvent(int button, int action, double mouse_x, double mouse_y) {
    double corner_x = getPos().at(0) + radius;
    double corner_y = getPos().at(1) + radius;
    double distance = std::sqrt((mouse_x - corner_x)*(mouse_x - corner_x) + (mouse_y - corner_y)*(mouse_y - corner_y));
    if (distance > radius)
        return;

    if (action == GLFW_PRESS) {
        std::cout << "[ButtonWidget] Button pressed!" << std::endl;
        if (toggleOnClick) {
            current_icon = (current_icon == ButtonIconType::Play)
                ? ButtonIconType::Pause
                : ButtonIconType::Play;

            char debug_char = (current_icon == ButtonIconType::Play) ? 'P' : 'S';

            int return_value = ::send(sock_fd, &debug_char, 1, 0);
            if (return_value < 0) {
                std::cerr << "[ButtonWidget] Failed to send command.\n";
            } else {
                std::cout << "[ButtonWidget] Sent command '" << debug_char << "' to server.\n";
            }
            if (current_icon == ButtonIconType::Play)
                std::cout << "[ButtonWidget] State changed to PLAY\n";
            else
                std::cout << "[ButtonWidget] State changed to PAUSE\n";
        }
    }
    WidgetClass::onMouseEvent(button, action, mouse_x, mouse_y);
}

TeardownButtonWidget::TeardownButtonWidget(double x, double y, double radius, int socket_fd)
    : WidgetClass(x - radius, y - radius, radius * 2, radius * 2),
      radius(radius),
      sock_fd(socket_fd),
      active(true) {}

void TeardownButtonWidget::render() {
    if (!active) {
        glColor3f(0.4f, 0.4f, 0.4f);
    } else if (isPressed()) {
        glColor3f(1.0f, 0.0f, 0.0f);
    } else {
        glColor3f(0.8f, 0.1f, 0.1f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(getPos().at(0) + radius, getPos().at(1) + radius);
    const int segments = 50;
    for (int index = 0; index <= segments; ++index) {
        float angle = index * 2.0f * PI / segments;
        float differential_x = cosf(angle) * radius;
        float dy = sinf(angle) * radius;
        glVertex2f(getPos().at(0) + radius + differential_x, getPos().at(1) + radius + dy);
    }
    glEnd();

    if (active) {
        glColor3f(1.0f, 1.0f, 1.0f);
        float corner_x = getPos().at(0) + radius;
        float corner_y = getPos().at(1) + radius;
        float half_radial_distance = radius * 0.5f;
        float thickness = 3.0f;

        glLineWidth(thickness);
        glBegin(GL_LINES);
        glVertex2f(corner_x - half_radial_distance, corner_y - half_radial_distance);
        glVertex2f(corner_x + half_radial_distance, corner_y + half_radial_distance);
        glVertex2f(corner_x - half_radial_distance, corner_y + half_radial_distance);
        glVertex2f(corner_x + half_radial_distance, corner_y - half_radial_distance);
        glEnd();
        glLineWidth(1.0f);
    }
}


void TeardownButtonWidget::onMouseEvent(int button, int action, double mouse_x, double mouse_y) {
    if (!active)
        return;
    double corner_x = getPos().at(0) + radius;
    double corner_y = getPos().at(1) + radius;
    double distance = std::sqrt((mouse_x - corner_x)*(mouse_x - corner_x) + (mouse_y - corner_y)*(mouse_y - corner_y));
    if (distance > radius)
        return;
    if (action == GLFW_PRESS) {
        std::cout << "[TeardownButtonWidget] Button pressed - tearing down connection.\n";

#ifdef _WIN32
        ::closesocket(sock_fd);
#else
        ::close(sock_fd);
#endif
        active = false;
    }
    WidgetClass::onMouseEvent(button, action, mouse_x, mouse_y);
}
