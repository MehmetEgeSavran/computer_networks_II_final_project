#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#define SCREEN_WIDTH 1920*0.85
#define SCREEN_HEIGHT 1080*0.85

class WindowClass {
public:
    WindowClass() {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW library" << std::endl;
            WINDOW_CLASS_ERROR = true;
            return;
        }
        window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL Window", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            WINDOW_CLASS_ERROR = true;
            return;
        }
        glfwMakeContextCurrent(window);
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            WINDOW_CLASS_ERROR = true;
        }
    }
    ~WindowClass() {
        if (window)
            glfwDestroyWindow(window);
        glfwTerminate();
    }
    GLFWwindow* getWindow() {
        return window;
    }
    bool getErrorStatus() {
        return WINDOW_CLASS_ERROR;
    }
    
private:
    bool WINDOW_CLASS_ERROR = false;
    GLFWwindow* window = nullptr;
};

class WidgetClass {
public:
    WidgetClass(double x, double y, double w, double h) {
        xpos = x; ypos = y; width = w; height = h;
    };
    ~WidgetClass() { }
    virtual void render() = 0;
    virtual void onMouseEvent(int button, int action, double mouse_x, double mouse_y) {
        if( !contains(mouse_x, mouse_y) )
            return;
        if(action == GLFW_PRESS) {
            pressed = true;
            // std::cout << "Widget has been pressed" << std::endl;
        } else if(action == GLFW_RELEASE) {
            pressed = false;
            // std::cout << "Widget has been released" << std::endl;
        }
    }
    bool contains(double x, double y) {
        return 
            (x >= xpos) && (x <= xpos + width) &&
            (y >= ypos) && (y <= ypos + height);
    }
    bool isPressed() const { return pressed; }
    std::vector<double> getPos() { return { xpos, ypos }; }
    std::vector<double> getDim() { return { height, width }; }

private:
    double xpos, ypos, height, width;
    bool pressed = false;
};

class WidgetManager {
public:
    void addNewWidget(WidgetClass* widget) {
        widgets.push_back(widget);
    }
    void renderAll() {
        for(auto w:widgets) {
            w->render();
        }
    }
    void handleMouseEvent(int button, int action, double x, double y) {
        for(auto w : widgets) {
            w->onMouseEvent(button, action, x, y);
        }
    }
private:
    std::vector<WidgetClass*> widgets;
};

class TestWidget : public WidgetClass {
public:
    TestWidget(double x, double y, double w, double h) : WidgetClass(x, y, w, h) {}
    void render() override {
        if(isPressed())
            glColor3f(1.0f, 0.0f, 0.0f);
        else 
            glColor3f(0.0f, 1.0f, 0.0f);

        glBegin(GL_QUADS);
        glVertex2f( getPos().at(0), getPos().at(1) );
        glVertex2f( getPos().at(0) + getDim().at(1), getPos().at(1) );
        glVertex2f( getPos().at(0) + getDim().at(1), getPos().at(1) + getDim().at(0) );
        glVertex2f( getPos().at(0), getPos().at(1) + getDim().at(0) );
        glEnd();
    }
};

class MouseClass {
public:
    MouseClass(GLFWwindow* window, WidgetManager* manager) {
        parent_window = window;
        widget_manager = manager;

        glfwSetWindowUserPointer(parent_window, this);
        glfwSetKeyCallback(parent_window, keyCallback);
        glfwSetMouseButtonCallback(parent_window, mousePressCallback);
        glfwSetCursorPosCallback(parent_window, mouseMoveCallback);
    }
    ~MouseClass() {}
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            // std::cout << "Key Pressed: " << key << std::endl;

            if (key == GLFW_KEY_ESCAPE) {
                glfwSetWindowShouldClose(window, true);
            }
        }
    }
    static void mousePressCallback(GLFWwindow* window, int button, int action, int mods) {
        MouseClass* instance = static_cast<MouseClass*>( glfwGetWindowUserPointer(window) );
        if(!instance)
            return;
        if(instance->widget_manager) {
            instance->widget_manager->handleMouseEvent(button, action, instance->xpos, instance->ypos);
        }
        if (action == GLFW_PRESS) {
            // std::cout << "Mouse Button Pressed: " << button << std::endl;
        }
    }
    static void mouseMoveCallback(GLFWwindow* window, double x, double y) {
        MouseClass* instance = static_cast<MouseClass*>( glfwGetWindowUserPointer(window) );
        if(!instance)
            return;
        instance->xpos = x;
        instance->ypos = y;
        // std::cout << "The cursor moved to [" << instance->xpos << "," << instance->ypos << "]" << std::endl; 
    }
private:
    WidgetManager* widget_manager = nullptr;
    GLFWwindow* parent_window = nullptr;
    double xpos = 0.0, ypos = 0.0;
};

int main() { 
    WindowClass custom_window;
    if(custom_window.getErrorStatus()) { return -1; }
    WidgetManager widget_manager;
    MouseClass custom_mouse(custom_window.getWindow(), &widget_manager);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    TestWidget myWidget(100, 100, 200, 100);
    widget_manager.addNewWidget(&myWidget);

    while (!glfwWindowShouldClose(custom_window.getWindow())) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glLoadIdentity();

        widget_manager.renderAll();

        glfwSwapBuffers(custom_window.getWindow());
        glfwPollEvents();
    }

    return 0;
}