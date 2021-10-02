#pragma once

#include <vector>
#include <GLFW/glfw3.h>

struct KeyEvent {
    GLFWwindow* window;
    int key;
    int scancode;
    int action;
    int mods;
};

struct CharEvent {
    GLFWwindow* window;
    unsigned int codepoint;
};

struct CharModsEvent {
    GLFWwindow* window;
    unsigned int codepoint;
    int mods;
};

struct MouseButtonEvent {
    GLFWwindow* window;
    int button;
    int action;
    int mods;
};

struct CursorPosEvent {
    GLFWwindow* window;
    double xpos;
    double ypos;
};

struct CursorEnterEvent {
    GLFWwindow* window;
    int entered;
};

struct ScrollEvent {
    GLFWwindow* window;
    double xoffset; 
    double yoffset; 
};

struct DropEvent {
    GLFWwindow* window;
    int count;
    const char** paths;
};

void initInput(GLFWwindow* window);
void updateInput();
void clearKeyboardInput();
void clearMouseInput();

KeyEvent getKey(int key);
double getMouseX();
double getMouseY();
MouseButtonEvent getMouseButton(int key);

std::vector<KeyEvent>& getKeyEvents();
std::vector<CharEvent>& getCharEvents();
std::vector<CharModsEvent>& getCharModsEvents();
std::vector<MouseButtonEvent>& getMouseButtonEvents();
std::vector<CursorPosEvent>& getCursorPosEvents();
std::vector<CursorEnterEvent>& getCursorEnterEvents();
std::vector<ScrollEvent>& getScrollEvents();
std::vector<DropEvent>& getDropEvents();
