#include "input.h"

struct InputState {

    KeyEvent lastKeyState[400];

    double cursorPosX = 0;
    double cursorPosY = 0;
    MouseButtonEvent lastMouseButtonState[8];

    bool hideKeyState = false;
    bool hideMouseButtonState = false;

    std::vector<KeyEvent> keyEvents;
    std::vector<CharEvent> charEvents;
    std::vector<CharModsEvent> charModsEvents;
    std::vector<MouseButtonEvent> mouseButtonEvents;
    std::vector<CursorPosEvent> cursorPosEvents;
    std::vector<CursorEnterEvent> cursorEnterEvents;
    std::vector<ScrollEvent> scrollEvents;
    std::vector<DropEvent> dropEvents;
};

InputState ia;
InputState ib;

InputState* iw = &ia;

void keyEventsCallback(
        GLFWwindow* window, int key, int scancode, int action, int mods) {

    if (action != GLFW_REPEAT) {
        iw->lastKeyState[key].window = window;
        iw->lastKeyState[key].action = action;
        iw->lastKeyState[key].key = key;
        iw->lastKeyState[key].scancode = scancode;
        iw->lastKeyState[key].mods = mods;
    }

    keyEvents.push_back(lastKeyState[key]);
}

void charEventsCallback(
        GLFWwindow* window, unsigned int codepoint) {

    charEvents.push_back({window, codepoint});
}

void charModsEventsCallback(
        GLFWwindow* window, unsigned int codepoint, int mods) {

    charModsEvents.push_back({window, codepoint, mods});
}

void mouseButtonEventsCallback(
        GLFWwindow* window, int button, int action, int mods) {

    lastMouseButtonState[button].window = window;
    lastMouseButtonState[button].button = button;
    lastMouseButtonState[button].action = action;
    lastMouseButtonState[button].mods = mods;

    mouseButtonEvents.push_back(lastMouseButtonState[button]);
}

void cursorPosEventsCallback(
        GLFWwindow* window, double xpos, double ypos) {

    cursorPosX = xpos;
    cursorPosY = ypos;

    cursorPosEvents.push_back({window, xpos, ypos});
}

void cursorEnterEventsCallback(
        GLFWwindow* window, int entered) {

    cursorEnterEvents.push_back({window, entered});
}

void scrollEventsCallback(
        GLFWwindow* window, double xoffset, double yoffset) {

    scrollEvents.push_back({window, xoffset, yoffset});
}

void dropEventsCallback(
        GLFWwindow* window, int count, const char** paths) {

    dropEvents.push_back({window, count, paths});
}

void initInput(GLFWwindow* window) {

    if (window == nullptr) {
        return;
    }

    glfwGetCursorPos(window, &cursorPosX, &cursorPosY);

    glfwSetKeyCallback(window, keyEventsCallback);
    glfwSetCharCallback(window, charEventsCallback);
    glfwSetCharModsCallback(window, charModsEventsCallback);
    glfwSetMouseButtonCallback(window, mouseButtonEventsCallback);
    glfwSetCursorPosCallback(window, cursorPosEventsCallback);
    glfwSetCursorEnterCallback(window, cursorEnterEventsCallback);
    glfwSetScrollCallback(window, scrollEventsCallback);
    glfwSetDropCallback(window, dropEventsCallback);
}

void updateInput() {

    hideKeyState = false;
    hideMouseButtonState = false;

    keyEvents.clear();
    charEvents.clear();
    charModsEvents.clear();
    mouseButtonEvents.clear();
    cursorPosEvents.clear();
    cursorEnterEvents.clear();
    scrollEvents.clear();
    dropEvents.clear();
    
    glfwPollEvents();
}

void clearKeyboardInput() {

    hideKeyState = true;

    keyEvents.clear();
    charEvents.clear();
    charModsEvents.clear();
}

void clearMouseInput() {

    hideMouseButtonState = true;

    mouseButtonEvents.clear();
    cursorPosEvents.clear();
    cursorEnterEvents.clear();
    scrollEvents.clear();
    dropEvents.clear();
}

KeyEvent getKey(int key) {

    if (hideKeyState) {
        return {lastKeyState[key].window,
                lastKeyState[key].key,
                lastKeyState[key].scancode,
                GLFW_RELEASE,
                0};
    } else {
        return lastKeyState[key];
    }
}

double getMouseX() {
    return cursorPosX;
}

double getMouseY() {
    return cursorPosY;
}

MouseButtonEvent getMouseButton(int button) {

    if (hideMouseButtonState) {
        return {lastMouseButtonState[button].window,
                lastMouseButtonState[button].button,
                GLFW_RELEASE,
                0};
    } else {
        return lastMouseButtonState[button];
    }
}

std::vector<KeyEvent>& getKeyEvents() {
    return keyEvents;
}

std::vector<CharEvent>& getCharEvents() {
    return charEvents;
}

std::vector<CharModsEvent>& getCharModsEvents() {
    return charModsEvents;
}

std::vector<MouseButtonEvent>& getMouseButtonEvents() {
    return mouseButtonEvents;
}

std::vector<CursorPosEvent>& getCursorPosEvents() {
    return cursorPosEvents;
}

std::vector<CursorEnterEvent>& getCursorEnterEvents() {
    return cursorEnterEvents;
}

std::vector<ScrollEvent>& getScrollEvents() {
    return scrollEvents;
}

std::vector<DropEvent>& getDropEvents() { 
    return dropEvents;
}
