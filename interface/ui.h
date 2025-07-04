#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

// ================== WindowClass ==================
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

// ================== WidgetClass (Base) ==================
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
    double x_position, y_position, width, height;
    bool pressed = false;
};

// ================== WidgetManager ==================
class WidgetManager {
public:
    void addNewWidget(WidgetClass* widget);
    void renderAll();
    void handleMouseEvent(int button, int action, double x, double y);

private:
    std::vector<WidgetClass*> widgets;
};

// ================== MouseClass ==================
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

// ================== ButtonWidget ==================
enum class ButtonIconType {
    Play,
    Pause
};

class ButtonWidget : public WidgetClass {
public:
    ButtonWidget(double x, double y, double radius, ButtonIconType icon, bool toggle, int socket_fd);

    void render() override;
    void onMouseEvent(int button, int action, double mouse_x, double mouse_y) override;

private:
    double radius;
    ButtonIconType currentIcon;
    bool toggleOnClick;
    int sock_fd;
};

// ----------------- TeardownButtonWidget -----------------
class TeardownButtonWidget : public WidgetClass {
public:
    TeardownButtonWidget(double x, double y, double radius, int socket_fd);
    void render() override;
    void onMouseEvent(int button, int action, double mouse_x, double mouse_y) override;
private:
    double radius;
    int sock_fd;
    bool active;
};
