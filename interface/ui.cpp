#include "ui.h"
#include <iostream>
#include <cmath>

// Add missing system headers
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <unistd.h>
#endif

// ----------------- WindowClass -----------------
WindowClass::WindowClass() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        WINDOW_CLASS_ERROR = true;
        return;
    }
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

// ----------------- WidgetClass -----------------
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

// ----------------- WidgetManager -----------------
void WidgetManager::addNewWidget(WidgetClass* widget) {
    widgets.push_back(widget);
}
void WidgetManager::renderAll() {
    for (auto w : widgets) {
        w->render();
    }
}
void WidgetManager::handleMouseEvent(int button, int action, double x, double y) {
    for (auto w : widgets) {
        w->onMouseEvent(button, action, x, y);
    }
}

// ----------------- MouseClass -----------------
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

// ----------------- ButtonWidget -----------------
ButtonWidget::ButtonWidget(double x, double y, double radius, ButtonIconType icon, bool toggle, int socket_fd)
    : WidgetClass(x - radius, y - radius, radius * 2, radius * 2),
      radius(radius),
      currentIcon(icon),
      toggleOnClick(toggle),
      sock_fd(socket_fd) {}

void ButtonWidget::render() {
    if (isPressed())
        glColor3f(1.0f, 0.3f, 0.3f);
    else
        glColor3f(0.3f, 0.8f, 0.3f);

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(getPos().at(0) + radius, getPos().at(1) + radius);
    const int segments = 50;
    for (int i = 0; i <= segments; ++i) {
        float angle = i * 2.0f * 3.1415926f / segments;
        float dx = cosf(angle) * radius;
        float dy = sinf(angle) * radius;
        glVertex2f(getPos().at(0) + radius + dx, getPos().at(1) + radius + dy);
    }
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    float cx = getPos().at(0) + radius;
    float cy = getPos().at(1) + radius;

    if (currentIcon == ButtonIconType::Play) {
        float s = radius * 0.6f;
        glBegin(GL_TRIANGLES);
        glVertex2f(cx - s * 0.4f, cy - s);
        glVertex2f(cx - s * 0.4f, cy + s);
        glVertex2f(cx + s * 0.6f, cy);
        glEnd();
    } else if (currentIcon == ButtonIconType::Pause) {
        float w = radius * 0.2f;
        float h = radius * 0.7f;
        glBegin(GL_QUADS);
        
        glVertex2f(cx - w - w, cy - h);
        glVertex2f(cx - w, cy - h);
        glVertex2f(cx - w, cy + h);
        glVertex2f(cx - w - w, cy + h);
        
        glVertex2f(cx + w, cy - h);
        glVertex2f(cx + w + w, cy - h);
        glVertex2f(cx + w + w, cy + h);
        glVertex2f(cx + w, cy + h);
        glEnd();
    }
}

void ButtonWidget::onMouseEvent(int button, int action, double mouse_x, double mouse_y) {
    double cx = getPos().at(0) + radius;
    double cy = getPos().at(1) + radius;
    double dist = std::sqrt((mouse_x - cx)*(mouse_x - cx) + (mouse_y - cy)*(mouse_y - cy));
    if (dist > radius)
        return;

    if (action == GLFW_PRESS) {
        std::cout << "[ButtonWidget] Button pressed!" << std::endl;
        if (toggleOnClick) {
            currentIcon = (currentIcon == ButtonIconType::Play)
                ? ButtonIconType::Pause
                : ButtonIconType::Play;

            char cmd = (currentIcon == ButtonIconType::Play) ? 'P' : 'S';

            int ret = ::send(sock_fd, &cmd, 1, 0);
            if (ret < 0) {
                std::cerr << "[ButtonWidget] Failed to send command.\n";
            } else {
                std::cout << "[ButtonWidget] Sent command '" << cmd << "' to server.\n";
            }

            if (currentIcon == ButtonIconType::Play)
                std::cout << "[ButtonWidget] State changed to PLAY\n";
            else
                std::cout << "[ButtonWidget] State changed to PAUSE\n";
        }
    }
    WidgetClass::onMouseEvent(button, action, mouse_x, mouse_y);
}

// ----------------- TeardownButtonWidget -----------------
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
    for (int i = 0; i <= segments; ++i) {
        float angle = i * 2.0f * 3.1415926f / segments;
        float dx = cosf(angle) * radius;
        float dy = sinf(angle) * radius;
        glVertex2f(getPos().at(0) + radius + dx, getPos().at(1) + radius + dy);
    }
    glEnd();

    // Cross icon
    if (active) {
        glColor3f(1.0f, 1.0f, 1.0f);
        float cx = getPos().at(0) + radius;
        float cy = getPos().at(1) + radius;
        float s = radius * 0.5f;
        float thickness = 3.0f;

        glLineWidth(thickness);
        glBegin(GL_LINES);
        // Diagonal top-left to bottom-right
        glVertex2f(cx - s, cy - s);
        glVertex2f(cx + s, cy + s);
        // Diagonal bottom-left to top-right
        glVertex2f(cx - s, cy + s);
        glVertex2f(cx + s, cy - s);
        glEnd();
        glLineWidth(1.0f);
    }
}


void TeardownButtonWidget::onMouseEvent(int button, int action, double mouse_x, double mouse_y) {
    if (!active)
        return;

    double cx = getPos().at(0) + radius;
    double cy = getPos().at(1) + radius;
    double dist = std::sqrt((mouse_x - cx)*(mouse_x - cx) + (mouse_y - cy)*(mouse_y - cy));
    if (dist > radius)
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
