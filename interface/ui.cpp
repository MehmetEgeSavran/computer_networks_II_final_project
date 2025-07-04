#include "ui.h"
#include <iostream>
#include <cmath>

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
    : xpos(x), ypos(y), width(w), height(h) {}

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
    return (x >= xpos) && (x <= xpos + width) && (y >= ypos) && (y <= ypos + height);
}

bool WidgetClass::isPressed() const { return pressed; }
std::vector<double> WidgetClass::getPos() { return { xpos, ypos }; }
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
ButtonWidget::ButtonWidget(double x, double y, double radius, ButtonIconType icon, bool toggle)
    : WidgetClass(x - radius, y - radius, radius * 2, radius * 2),
      radius(radius),
      currentIcon(icon),
      toggleOnClick(toggle) {}

void ButtonWidget::render() {
    // Draw circle background
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

    // Draw icon
    glColor3f(1.0f, 1.0f, 1.0f);
    float cx = getPos().at(0) + radius;
    float cy = getPos().at(1) + radius;

    if (currentIcon == ButtonIconType::Play) {
        // Triangle (play)
        float s = radius * 0.6f;
        glBegin(GL_TRIANGLES);
        glVertex2f(cx - s * 0.4f, cy - s);
        glVertex2f(cx - s * 0.4f, cy + s);
        glVertex2f(cx + s * 0.6f, cy);
        glEnd();
    } else if (currentIcon == ButtonIconType::Pause) {
        // Two bars (pause)
        float w = radius * 0.2f;
        float h = radius * 0.7f;
        glBegin(GL_QUADS);
        // Left bar
        glVertex2f(cx - w - w, cy - h);
        glVertex2f(cx - w, cy - h);
        glVertex2f(cx - w, cy + h);
        glVertex2f(cx - w - w, cy + h);
        // Right bar
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
    double dist = std::sqrt((mouse_x - cx) * (mouse_x - cx) + (mouse_y - cy) * (mouse_y - cy));
    if (dist > radius)
        return;

    if (action == GLFW_PRESS) {
        std::cout << "[ButtonWidget] Button pressed!" << std::endl;
        if (toggleOnClick) {
            currentIcon = (currentIcon == ButtonIconType::Play)
                ? ButtonIconType::Pause
                : ButtonIconType::Play;
        }
    }
    WidgetClass::onMouseEvent(button, action, mouse_x, mouse_y);
}
