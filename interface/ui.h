#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

// Window wrapper
class WindowClass {
public:
    WindowClass();
    ~WindowClass();
    GLFWwindow* getWindow();
    bool getErrorStatus();
private:
    bool WINDOW_CLASS_ERROR = false;
    GLFWwindow* window = nullptr;
};

// Abstract base widget
class WidgetClass {
public:
    WidgetClass(double x, double y, double w, double h);
    virtual ~WidgetClass();
    virtual void render() = 0;
    virtual void onMouseEvent(int button, int action, double mouse_x, double mouse_y);
    bool contains(double x, double y);
    bool isPressed() const;
    std::vector<double> getPos();
    std::vector<double> getDim();
protected:
    double xpos, ypos, width, height;
    bool pressed = false;
};

// Widget manager
class WidgetManager {
public:
    void addNewWidget(WidgetClass* widget);
    void renderAll();
    void handleMouseEvent(int button, int action, double x, double y);
private:
    std::vector<WidgetClass*> widgets;
};

// Mouse input
class MouseClass {
public:
    MouseClass(GLFWwindow* window, WidgetManager* manager);
    ~MouseClass();
private:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mousePressCallback(GLFWwindow* window, int button, int action, int mods);
    static void mouseMoveCallback(GLFWwindow* window, double x, double y);
    WidgetManager* widget_manager = nullptr;
    GLFWwindow* parent_window = nullptr;
    double xpos = 0.0, ypos = 0.0;
};
