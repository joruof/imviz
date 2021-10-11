#include "input.h"

#include <mutex>

#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/pytypes.h>

namespace py = pybind11;

namespace input {

struct state {

    KeyEvent lastKeyState[400];

    double cursorPosX = 0;
    double cursorPosY = 0;
    MouseButtonEvent lastMouseButtonState[8];

    std::vector<KeyEvent> keyEvents;
    std::vector<CharEvent> charEvents;
    std::vector<CharModsEvent> charModsEvents;
    std::vector<MouseButtonEvent> mouseButtonEvents;
    std::vector<CursorPosEvent> cursorPosEvents;
    std::vector<CursorEnterEvent> cursorEnterEvents;
    std::vector<ScrollEvent> scrollEvents;
    std::vector<DropEvent> dropEvents;
};

/**
 * Double buffering the input state ensures that the return values
 * of the input getter functions are consistent between calls.
 */
state inputStateA;
state inputStateB;

state* writeState = &inputStateA;
state* readState = &inputStateB;

/**
 * Lock this before accessing the write state
 */
std::mutex writeStateMutex;

void keyEventsCallback(
        GLFWwindow* window, int key, int scancode, int action, int mods) {

    std::lock_guard<std::mutex> lock(writeStateMutex);

    writeState->lastKeyState[key].window = window;
    writeState->lastKeyState[key].action = action;
    writeState->lastKeyState[key].key = key;
    writeState->lastKeyState[key].scancode = scancode;
    writeState->lastKeyState[key].mods = mods;

    writeState->keyEvents.push_back(writeState->lastKeyState[key]);
}

void charEventsCallback(
        GLFWwindow* window, unsigned int codepoint) {

    std::lock_guard<std::mutex> lock(writeStateMutex);

    writeState->charEvents.push_back({window, codepoint});
}

void charModsEventsCallback(
        GLFWwindow* window, unsigned int codepoint, int mods) {

    std::lock_guard<std::mutex> lock(writeStateMutex);

    writeState->charModsEvents.push_back({window, codepoint, mods});
}

void mouseButtonEventsCallback(
        GLFWwindow* window, int button, int action, int mods) {

    std::lock_guard<std::mutex> lock(writeStateMutex);

    writeState->lastMouseButtonState[button].window = window;
    writeState->lastMouseButtonState[button].button = button;
    writeState->lastMouseButtonState[button].action = action;
    writeState->lastMouseButtonState[button].mods = mods;

    writeState->mouseButtonEvents.push_back(
            writeState->lastMouseButtonState[button]);
}

void cursorPosEventsCallback(
        GLFWwindow* window, double xpos, double ypos) {

    std::lock_guard<std::mutex> lock(writeStateMutex);

    writeState->cursorPosX = xpos;
    writeState->cursorPosY = ypos;

    writeState->cursorPosEvents.push_back({window, xpos, ypos});
}

void cursorEnterEventsCallback(
        GLFWwindow* window, int entered) {

    std::lock_guard<std::mutex> lock(writeStateMutex);

    writeState->cursorEnterEvents.push_back({window, entered});
}

void scrollEventsCallback(
        GLFWwindow* window, double xoffset, double yoffset) {

    std::lock_guard<std::mutex> lock(writeStateMutex);

    writeState->scrollEvents.push_back({window, xoffset, yoffset});
}

void dropEventsCallback(
        GLFWwindow* window, int count, const char** paths) {

    std::lock_guard<std::mutex> lock(writeStateMutex);
    
    DropEvent& event = writeState->dropEvents.emplace_back();
    event.window = window;

    for (int i = 0; i < count; ++i) {
        event.paths.push_back(std::string(paths[i]));
    }
}

void registerCallbacks(GLFWwindow* window) {

    for (int i = 0; i < 400; ++i) {
        writeState->lastKeyState[i].key = i;
        readState->lastKeyState[i].key = i;
    }

    glfwSetKeyCallback(window, keyEventsCallback);
    glfwSetCharCallback(window, charEventsCallback);
    glfwSetCharModsCallback(window, charModsEventsCallback);
    glfwSetMouseButtonCallback(window, mouseButtonEventsCallback);
    glfwSetCursorPosCallback(window, cursorPosEventsCallback);
    glfwSetCursorEnterCallback(window, cursorEnterEventsCallback);
    glfwSetScrollCallback(window, scrollEventsCallback);
    glfwSetDropCallback(window, dropEventsCallback);
}

void update() {

    std::lock_guard<std::mutex> lock(writeStateMutex);

    state* tmpStateRef = writeState;
    writeState = readState;
    readState = tmpStateRef;

    writeState->keyEvents.clear();
    writeState->charEvents.clear();
    writeState->charModsEvents.clear();
    writeState->mouseButtonEvents.clear();
    writeState->cursorPosEvents.clear();
    writeState->cursorEnterEvents.clear();
    writeState->scrollEvents.clear();
    writeState->dropEvents.clear();
}

void clearKeyboardInput() {

    for (int i = 0; i < 400; ++i) {
        readState->lastKeyState[i].action = GLFW_RELEASE;
    }

    readState->keyEvents.clear();
    readState->charEvents.clear();
    readState->charModsEvents.clear();
}

void clearMouseInput() {

    for (int i = 0; i < 8; ++i) {
        readState->lastMouseButtonState[i].action = GLFW_RELEASE;
    }

    readState->mouseButtonEvents.clear();
    readState->cursorPosEvents.clear();
    readState->cursorEnterEvents.clear();
    readState->scrollEvents.clear();
    readState->dropEvents.clear();
}

KeyEvent getKey(int key) {

    return readState->lastKeyState[key];
}

MouseButtonEvent getMouseButton(int button) {

    return readState->lastMouseButtonState[button];
}

double getCursorX() {
    return readState->cursorPosX;
}

double getCursorY() {
    return readState->cursorPosY;
}

std::vector<KeyEvent>& getKeyEvents() {
    return readState->keyEvents;
}

std::vector<CharEvent>& getCharEvents() {
    return readState->charEvents;
}

std::vector<CharModsEvent>& getCharModsEvents() {
    return readState->charModsEvents;
}

std::vector<MouseButtonEvent>& getMouseButtonEvents() {
    return readState->mouseButtonEvents;
}

std::vector<CursorPosEvent>& getCursorPosEvents() {
    return readState->cursorPosEvents;
}

std::vector<CursorEnterEvent>& getCursorEnterEvents() {
    return readState->cursorEnterEvents;
}

std::vector<ScrollEvent>& getScrollEvents() {
    return readState->scrollEvents;
}

std::vector<DropEvent>& getDropEvents() { 
    return readState->dropEvents;
}

void loadPythonBindings(pybind11::module& m) {

    /*
     * GLFW Constants
     */

    m.add_object("VERSION_MAJOR", py::int_(3));
    m.add_object("VERSION_MINOR", py::int_(3));
    m.add_object("VERSION_REVISION", py::int_(5));
    m.add_object("TRUE", py::int_(1));
    m.add_object("FALSE", py::int_(0));
    m.add_object("RELEASE", py::int_(0));
    m.add_object("PRESS", py::int_(1));
    m.add_object("REPEAT", py::int_(2));
    m.add_object("HAT_CENTERED", py::int_(0));
    m.add_object("HAT_UP", py::int_(1));
    m.add_object("HAT_RIGHT", py::int_(2));
    m.add_object("HAT_DOWN", py::int_(4));
    m.add_object("HAT_LEFT", py::int_(8));
    m.add_object("KEY_UNKNOWN", py::int_(-1));
    m.add_object("KEY_SPACE", py::int_(32));
    m.add_object("KEY_0", py::int_(48));
    m.add_object("KEY_1", py::int_(49));
    m.add_object("KEY_2", py::int_(50));
    m.add_object("KEY_3", py::int_(51));
    m.add_object("KEY_4", py::int_(52));
    m.add_object("KEY_5", py::int_(53));
    m.add_object("KEY_6", py::int_(54));
    m.add_object("KEY_7", py::int_(55));
    m.add_object("KEY_8", py::int_(56));
    m.add_object("KEY_9", py::int_(57));
    m.add_object("KEY_A", py::int_(65));
    m.add_object("KEY_B", py::int_(66));
    m.add_object("KEY_C", py::int_(67));
    m.add_object("KEY_D", py::int_(68));
    m.add_object("KEY_E", py::int_(69));
    m.add_object("KEY_F", py::int_(70));
    m.add_object("KEY_G", py::int_(71));
    m.add_object("KEY_H", py::int_(72));
    m.add_object("KEY_I", py::int_(73));
    m.add_object("KEY_J", py::int_(74));
    m.add_object("KEY_K", py::int_(75));
    m.add_object("KEY_L", py::int_(76));
    m.add_object("KEY_M", py::int_(77));
    m.add_object("KEY_N", py::int_(78));
    m.add_object("KEY_O", py::int_(79));
    m.add_object("KEY_P", py::int_(80));
    m.add_object("KEY_Q", py::int_(81));
    m.add_object("KEY_R", py::int_(82));
    m.add_object("KEY_S", py::int_(83));
    m.add_object("KEY_T", py::int_(84));
    m.add_object("KEY_U", py::int_(85));
    m.add_object("KEY_V", py::int_(86));
    m.add_object("KEY_W", py::int_(87));
    m.add_object("KEY_X", py::int_(88));
    m.add_object("KEY_Y", py::int_(89));
    m.add_object("KEY_Z", py::int_(90));
    m.add_object("KEY_ESCAPE", py::int_(256));
    m.add_object("KEY_ENTER", py::int_(257));
    m.add_object("KEY_TAB", py::int_(258));
    m.add_object("KEY_BACKSPACE", py::int_(259));
    m.add_object("KEY_INSERT", py::int_(260));
    m.add_object("KEY_DELETE", py::int_(261));
    m.add_object("KEY_RIGHT", py::int_(262));
    m.add_object("KEY_LEFT", py::int_(263));
    m.add_object("KEY_DOWN", py::int_(264));
    m.add_object("KEY_UP", py::int_(265));
    m.add_object("KEY_PAGE_UP", py::int_(266));
    m.add_object("KEY_PAGE_DOWN", py::int_(267));
    m.add_object("KEY_HOME", py::int_(268));
    m.add_object("KEY_END", py::int_(269));
    m.add_object("KEY_CAPS_LOCK", py::int_(280));
    m.add_object("KEY_SCROLL_LOCK", py::int_(281));
    m.add_object("KEY_NUM_LOCK", py::int_(282));
    m.add_object("KEY_PRINT_SCREEN", py::int_(283));
    m.add_object("KEY_PAUSE", py::int_(284));
    m.add_object("KEY_F1", py::int_(290));
    m.add_object("KEY_F2", py::int_(291));
    m.add_object("KEY_F3", py::int_(292));
    m.add_object("KEY_F4", py::int_(293));
    m.add_object("KEY_F5", py::int_(294));
    m.add_object("KEY_F6", py::int_(295));
    m.add_object("KEY_F7", py::int_(296));
    m.add_object("KEY_F8", py::int_(297));
    m.add_object("KEY_F9", py::int_(298));
    m.add_object("KEY_F10", py::int_(299));
    m.add_object("KEY_F11", py::int_(300));
    m.add_object("KEY_F12", py::int_(301));
    m.add_object("KEY_F13", py::int_(302));
    m.add_object("KEY_F14", py::int_(303));
    m.add_object("KEY_F15", py::int_(304));
    m.add_object("KEY_F16", py::int_(305));
    m.add_object("KEY_F17", py::int_(306));
    m.add_object("KEY_F18", py::int_(307));
    m.add_object("KEY_F19", py::int_(308));
    m.add_object("KEY_F20", py::int_(309));
    m.add_object("KEY_F21", py::int_(310));
    m.add_object("KEY_F22", py::int_(311));
    m.add_object("KEY_F23", py::int_(312));
    m.add_object("KEY_F24", py::int_(313));
    m.add_object("KEY_F25", py::int_(314));
    m.add_object("KEY_KP_0", py::int_(320));
    m.add_object("KEY_KP_1", py::int_(321));
    m.add_object("KEY_KP_2", py::int_(322));
    m.add_object("KEY_KP_3", py::int_(323));
    m.add_object("KEY_KP_4", py::int_(324));
    m.add_object("KEY_KP_5", py::int_(325));
    m.add_object("KEY_KP_6", py::int_(326));
    m.add_object("KEY_KP_7", py::int_(327));
    m.add_object("KEY_KP_8", py::int_(328));
    m.add_object("KEY_KP_9", py::int_(329));
    m.add_object("KEY_KP_DECIMAL", py::int_(330));
    m.add_object("KEY_KP_DIVIDE", py::int_(331));
    m.add_object("KEY_KP_MULTIPLY", py::int_(332));
    m.add_object("KEY_KP_SUBTRACT", py::int_(333));
    m.add_object("KEY_KP_ADD", py::int_(334));
    m.add_object("KEY_KP_ENTER", py::int_(335));
    m.add_object("KEY_KP_EQUAL", py::int_(336));
    m.add_object("KEY_LEFT_SHIFT", py::int_(340));
    m.add_object("KEY_LEFT_CONTROL", py::int_(341));
    m.add_object("KEY_LEFT_ALT", py::int_(342));
    m.add_object("KEY_LEFT_SUPER", py::int_(343));
    m.add_object("KEY_RIGHT_SHIFT", py::int_(344));
    m.add_object("KEY_RIGHT_CONTROL", py::int_(345));
    m.add_object("KEY_RIGHT_ALT", py::int_(346));
    m.add_object("KEY_RIGHT_SUPER", py::int_(347));
    m.add_object("KEY_MENU", py::int_(348));
    m.add_object("KEY_LAST", py::int_(GLFW_KEY_MENU));
    m.add_object("MOD_SHIFT", py::int_(0x0001));
    m.add_object("MOD_CONTROL", py::int_(0x0002));
    m.add_object("MOD_ALT", py::int_(0x0004));
    m.add_object("MOD_SUPER", py::int_(0x0008));
    m.add_object("MOD_CAPS_LOCK", py::int_(0x0010));
    m.add_object("MOD_NUM_LOCK", py::int_(0x0020));
    m.add_object("MOUSE_BUTTON_1", py::int_(0));
    m.add_object("MOUSE_BUTTON_2", py::int_(1));
    m.add_object("MOUSE_BUTTON_3", py::int_(2));
    m.add_object("MOUSE_BUTTON_4", py::int_(3));
    m.add_object("MOUSE_BUTTON_5", py::int_(4));
    m.add_object("MOUSE_BUTTON_6", py::int_(5));
    m.add_object("MOUSE_BUTTON_7", py::int_(6));
    m.add_object("MOUSE_BUTTON_8", py::int_(7));
    m.add_object("MOUSE_BUTTON_LAST", py::int_(GLFW_MOUSE_BUTTON_8));
    m.add_object("MOUSE_BUTTON_LEFT", py::int_(GLFW_MOUSE_BUTTON_1));
    m.add_object("MOUSE_BUTTON_RIGHT", py::int_(GLFW_MOUSE_BUTTON_2));
    m.add_object("MOUSE_BUTTON_MIDDLE", py::int_(GLFW_MOUSE_BUTTON_3));
    m.add_object("JOYSTICK_1", py::int_(0));
    m.add_object("JOYSTICK_2", py::int_(1));
    m.add_object("JOYSTICK_3", py::int_(2));
    m.add_object("JOYSTICK_4", py::int_(3));
    m.add_object("JOYSTICK_5", py::int_(4));
    m.add_object("JOYSTICK_6", py::int_(5));
    m.add_object("JOYSTICK_7", py::int_(6));
    m.add_object("JOYSTICK_8", py::int_(7));
    m.add_object("JOYSTICK_9", py::int_(8));
    m.add_object("JOYSTICK_10", py::int_(9));
    m.add_object("JOYSTICK_11", py::int_(10));
    m.add_object("JOYSTICK_12", py::int_(11));
    m.add_object("JOYSTICK_13", py::int_(12));
    m.add_object("JOYSTICK_14", py::int_(13));
    m.add_object("JOYSTICK_15", py::int_(14));
    m.add_object("JOYSTICK_16", py::int_(15));
    m.add_object("JOYSTICK_LAST", py::int_(GLFW_JOYSTICK_16));
    m.add_object("GAMEPAD_BUTTON_A", py::int_(0));
    m.add_object("GAMEPAD_BUTTON_B", py::int_(1));
    m.add_object("GAMEPAD_BUTTON_X", py::int_(2));
    m.add_object("GAMEPAD_BUTTON_Y", py::int_(3));
    m.add_object("GAMEPAD_BUTTON_LEFT_BUMPER", py::int_(4));
    m.add_object("GAMEPAD_BUTTON_RIGHT_BUMPER", py::int_(5));
    m.add_object("GAMEPAD_BUTTON_BACK", py::int_(6));
    m.add_object("GAMEPAD_BUTTON_START", py::int_(7));
    m.add_object("GAMEPAD_BUTTON_GUIDE", py::int_(8));
    m.add_object("GAMEPAD_BUTTON_LEFT_THUMB", py::int_(9));
    m.add_object("GAMEPAD_BUTTON_RIGHT_THUMB", py::int_(10));
    m.add_object("GAMEPAD_BUTTON_DPAD_UP", py::int_(11));
    m.add_object("GAMEPAD_BUTTON_DPAD_RIGHT", py::int_(12));
    m.add_object("GAMEPAD_BUTTON_DPAD_DOWN", py::int_(13));
    m.add_object("GAMEPAD_BUTTON_DPAD_LEFT", py::int_(14));
    m.add_object("GAMEPAD_BUTTON_LAST", py::int_(GLFW_GAMEPAD_BUTTON_DPAD_LEFT));
    m.add_object("GAMEPAD_BUTTON_CROSS", py::int_(GLFW_GAMEPAD_BUTTON_A));
    m.add_object("GAMEPAD_BUTTON_CIRCLE", py::int_(GLFW_GAMEPAD_BUTTON_B));
    m.add_object("GAMEPAD_BUTTON_SQUARE", py::int_(GLFW_GAMEPAD_BUTTON_X));
    m.add_object("GAMEPAD_BUTTON_TRIANGLE", py::int_(GLFW_GAMEPAD_BUTTON_Y));
    m.add_object("GAMEPAD_AXIS_LEFT_X", py::int_(0));
    m.add_object("GAMEPAD_AXIS_LEFT_Y", py::int_(1));
    m.add_object("GAMEPAD_AXIS_RIGHT_X", py::int_(2));
    m.add_object("GAMEPAD_AXIS_RIGHT_Y", py::int_(3));
    m.add_object("GAMEPAD_AXIS_LEFT_TRIGGER", py::int_(4));
    m.add_object("GAMEPAD_AXIS_RIGHT_TRIGGER", py::int_(5));
    m.add_object("GAMEPAD_AXIS_LAST", py::int_(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER));

    py::class_<KeyEvent>(m, "KeyEvent")
        .def(py::init<>())
        .def_readwrite("key", &KeyEvent::key)
        .def_readwrite("scancode", &KeyEvent::scancode)
        .def_readwrite("action", &KeyEvent::action)
        .def_readwrite("mod", &KeyEvent::mods);

    py::class_<CharEvent>(m, "CharEvent")
        .def(py::init<>())
        .def_readwrite("codepoint", &CharEvent::codepoint);

    py::class_<CharModsEvent>(m, "CharModsEvent")
        .def(py::init<>())
        .def_readwrite("codepoint", &CharModsEvent::codepoint)
        .def_readwrite("mods", &CharModsEvent::mods);

    py::class_<MouseButtonEvent>(m, "MouseButtonEvent")
        .def(py::init<>())
        .def_readwrite("button", &MouseButtonEvent::button)
        .def_readwrite("action", &MouseButtonEvent::action)
        .def_readwrite("mod", &MouseButtonEvent::mods);

    py::class_<CursorPosEvent>(m, "CursorPosEvent")
        .def(py::init<>())
        .def_readwrite("xpos", &CursorPosEvent::xpos)
        .def_readwrite("ypos", &CursorPosEvent::ypos);

    py::class_<CursorEnterEvent>(m, "CursorEnterEvent")
        .def(py::init<>())
        .def_readwrite("entered", &CursorEnterEvent::entered);

    py::class_<ScrollEvent>(m, "ScrollEvent")
        .def(py::init<>())
        .def_readwrite("xoffset", &ScrollEvent::xoffset)
        .def_readwrite("yoffset", &ScrollEvent::yoffset);

    py::class_<DropEvent>(m, "DropEvent")
        .def(py::init<>())
        .def_readonly("count", &DropEvent::paths);

    m.def("get_key", getKey);

    m.def("get_mouse_button", getMouseButton);
    m.def("get_mouse_pos", [&]() {
        return py::make_tuple(getCursorX(), getCursorY());
    });

    m.def("get_key_events", getKeyEvents);
    m.def("get_char_events", getCharEvents);
    m.def("get_char_mods_events", getCharModsEvents);
    m.def("get_mouse_button_events", getMouseButtonEvents);
    m.def("get_mouse_pos_events", getCursorPosEvents);
    m.def("get_mouse_enter_events", getCursorEnterEvents);
    m.def("get_scroll_events", getScrollEvents);
    m.def("get_drop_events", getDropEvents);
}

}
