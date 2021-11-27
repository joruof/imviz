#pragma once

#include <vector>
#include <GLFW/glfw3.h>

#include <pybind11/pybind11.h>

namespace input {

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
    std::vector<std::string> paths;
};

void registerCallbacks(GLFWwindow* window);

void update();
void clearKeyboardInput();
void clearMouseInput();

KeyEvent getKey(int key);

MouseButtonEvent getMouseButton(int key);
double getCursorX();
double getCursorY();

std::vector<KeyEvent>& getKeyEvents();
std::vector<CharEvent>& getCharEvents();
std::vector<CharModsEvent>& getCharModsEvents();
std::vector<MouseButtonEvent>& getMouseButtonEvents();
std::vector<CursorPosEvent>& getCursorPosEvents();
std::vector<CursorEnterEvent>& getCursorEnterEvents();
std::vector<ScrollEvent>& getScrollEvents();
std::vector<DropEvent>& getDropEvents();

void loadPythonBindings(pybind11::module& m);

}
