#include "input.hpp"

#include <mutex>

#include <nanobind/nanobind.h>

namespace nb = nanobind;

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

void loadPythonBindings(nanobind::module_& m) {

    /*
     * GLFW Constants
     */

    m.attr("VERSION_MAJOR") = nb::int_(3);
    m.attr("VERSION_MINOR") = nb::int_(3);
    m.attr("VERSION_REVISION") = nb::int_(5);
    m.attr("TRUE") = nb::int_(1);
    m.attr("FALSE") = nb::int_(0);
    m.attr("RELEASE") = nb::int_(0);
    m.attr("PRESS") = nb::int_(1);
    m.attr("REPEAT") = nb::int_(2);
    m.attr("HAT_CENTERED") = nb::int_(0);
    m.attr("HAT_UP") = nb::int_(1);
    m.attr("HAT_RIGHT") = nb::int_(2);
    m.attr("HAT_DOWN") = nb::int_(4);
    m.attr("HAT_LEFT") = nb::int_(8);
    m.attr("KEY_UNKNOWN") = nb::int_(-1);
    m.attr("KEY_SPACE") = nb::int_(32);
    m.attr("KEY_0") = nb::int_(48);
    m.attr("KEY_1") = nb::int_(49);
    m.attr("KEY_2") = nb::int_(50);
    m.attr("KEY_3") = nb::int_(51);
    m.attr("KEY_4") = nb::int_(52);
    m.attr("KEY_5") = nb::int_(53);
    m.attr("KEY_6") = nb::int_(54);
    m.attr("KEY_7") = nb::int_(55);
    m.attr("KEY_8") = nb::int_(56);
    m.attr("KEY_9") = nb::int_(57);
    m.attr("KEY_A") = nb::int_(65);
    m.attr("KEY_B") = nb::int_(66);
    m.attr("KEY_C") = nb::int_(67);
    m.attr("KEY_D") = nb::int_(68);
    m.attr("KEY_E") = nb::int_(69);
    m.attr("KEY_F") = nb::int_(70);
    m.attr("KEY_G") = nb::int_(71);
    m.attr("KEY_H") = nb::int_(72);
    m.attr("KEY_I") = nb::int_(73);
    m.attr("KEY_J") = nb::int_(74);
    m.attr("KEY_K") = nb::int_(75);
    m.attr("KEY_L") = nb::int_(76);
    m.attr("KEY_M") = nb::int_(77);
    m.attr("KEY_N") = nb::int_(78);
    m.attr("KEY_O") = nb::int_(79);
    m.attr("KEY_P") = nb::int_(80);
    m.attr("KEY_Q") = nb::int_(81);
    m.attr("KEY_R") = nb::int_(82);
    m.attr("KEY_S") = nb::int_(83);
    m.attr("KEY_T") = nb::int_(84);
    m.attr("KEY_U") = nb::int_(85);
    m.attr("KEY_V") = nb::int_(86);
    m.attr("KEY_W") = nb::int_(87);
    m.attr("KEY_X") = nb::int_(88);
    m.attr("KEY_Y") = nb::int_(89);
    m.attr("KEY_Z") = nb::int_(90);
    m.attr("KEY_ESCAPE") = nb::int_(256);
    m.attr("KEY_ENTER") = nb::int_(257);
    m.attr("KEY_TAB") = nb::int_(258);
    m.attr("KEY_BACKSPACE") = nb::int_(259);
    m.attr("KEY_INSERT") = nb::int_(260);
    m.attr("KEY_DELETE") = nb::int_(261);
    m.attr("KEY_RIGHT") = nb::int_(262);
    m.attr("KEY_LEFT") = nb::int_(263);
    m.attr("KEY_DOWN") = nb::int_(264);
    m.attr("KEY_UP") = nb::int_(265);
    m.attr("KEY_PAGE_UP") = nb::int_(266);
    m.attr("KEY_PAGE_DOWN") = nb::int_(267);
    m.attr("KEY_HOME") = nb::int_(268);
    m.attr("KEY_END") = nb::int_(269);
    m.attr("KEY_CAPS_LOCK") = nb::int_(280);
    m.attr("KEY_SCROLL_LOCK") = nb::int_(281);
    m.attr("KEY_NUM_LOCK") = nb::int_(282);
    m.attr("KEY_PRINT_SCREEN") = nb::int_(283);
    m.attr("KEY_PAUSE") = nb::int_(284);
    m.attr("KEY_F1") = nb::int_(290);
    m.attr("KEY_F2") = nb::int_(291);
    m.attr("KEY_F3") = nb::int_(292);
    m.attr("KEY_F4") = nb::int_(293);
    m.attr("KEY_F5") = nb::int_(294);
    m.attr("KEY_F6") = nb::int_(295);
    m.attr("KEY_F7") = nb::int_(296);
    m.attr("KEY_F8") = nb::int_(297);
    m.attr("KEY_F9") = nb::int_(298);
    m.attr("KEY_F10") = nb::int_(299);
    m.attr("KEY_F11") = nb::int_(300);
    m.attr("KEY_F12") = nb::int_(301);
    m.attr("KEY_F13") = nb::int_(302);
    m.attr("KEY_F14") = nb::int_(303);
    m.attr("KEY_F15") = nb::int_(304);
    m.attr("KEY_F16") = nb::int_(305);
    m.attr("KEY_F17") = nb::int_(306);
    m.attr("KEY_F18") = nb::int_(307);
    m.attr("KEY_F19") = nb::int_(308);
    m.attr("KEY_F20") = nb::int_(309);
    m.attr("KEY_F21") = nb::int_(310);
    m.attr("KEY_F22") = nb::int_(311);
    m.attr("KEY_F23") = nb::int_(312);
    m.attr("KEY_F24") = nb::int_(313);
    m.attr("KEY_F25") = nb::int_(314);
    m.attr("KEY_KP_0") = nb::int_(320);
    m.attr("KEY_KP_1") = nb::int_(321);
    m.attr("KEY_KP_2") = nb::int_(322);
    m.attr("KEY_KP_3") = nb::int_(323);
    m.attr("KEY_KP_4") = nb::int_(324);
    m.attr("KEY_KP_5") = nb::int_(325);
    m.attr("KEY_KP_6") = nb::int_(326);
    m.attr("KEY_KP_7") = nb::int_(327);
    m.attr("KEY_KP_8") = nb::int_(328);
    m.attr("KEY_KP_9") = nb::int_(329);
    m.attr("KEY_KP_DECIMAL") = nb::int_(330);
    m.attr("KEY_KP_DIVIDE") = nb::int_(331);
    m.attr("KEY_KP_MULTIPLY") = nb::int_(332);
    m.attr("KEY_KP_SUBTRACT") = nb::int_(333);
    m.attr("KEY_KP_ADD") = nb::int_(334);
    m.attr("KEY_KP_ENTER") = nb::int_(335);
    m.attr("KEY_KP_EQUAL") = nb::int_(336);
    m.attr("KEY_LEFT_SHIFT") = nb::int_(340);
    m.attr("KEY_LEFT_CONTROL") = nb::int_(341);
    m.attr("KEY_LEFT_ALT") = nb::int_(342);
    m.attr("KEY_LEFT_SUPER") = nb::int_(343);
    m.attr("KEY_RIGHT_SHIFT") = nb::int_(344);
    m.attr("KEY_RIGHT_CONTROL") = nb::int_(345);
    m.attr("KEY_RIGHT_ALT") = nb::int_(346);
    m.attr("KEY_RIGHT_SUPER") = nb::int_(347);
    m.attr("KEY_MENU") = nb::int_(348);
    m.attr("KEY_LAST") = nb::int_(GLFW_KEY_MENU);
    m.attr("MOD_SHIFT") = nb::int_(0x0001);
    m.attr("MOD_CONTROL") = nb::int_(0x0002);
    m.attr("MOD_ALT") = nb::int_(0x0004);
    m.attr("MOD_SUPER") = nb::int_(0x0008);
    m.attr("MOD_CAPS_LOCK") = nb::int_(0x0010);
    m.attr("MOD_NUM_LOCK") = nb::int_(0x0020);
    m.attr("MOUSE_BUTTON_1") = nb::int_(0);
    m.attr("MOUSE_BUTTON_2") = nb::int_(1);
    m.attr("MOUSE_BUTTON_3") = nb::int_(2);
    m.attr("MOUSE_BUTTON_4") = nb::int_(3);
    m.attr("MOUSE_BUTTON_5") = nb::int_(4);
    m.attr("MOUSE_BUTTON_6") = nb::int_(5);
    m.attr("MOUSE_BUTTON_7") = nb::int_(6);
    m.attr("MOUSE_BUTTON_8") = nb::int_(7);
    m.attr("MOUSE_BUTTON_LAST") = nb::int_(GLFW_MOUSE_BUTTON_8);
    m.attr("MOUSE_BUTTON_LEFT") = nb::int_(GLFW_MOUSE_BUTTON_1);
    m.attr("MOUSE_BUTTON_RIGHT") = nb::int_(GLFW_MOUSE_BUTTON_2);
    m.attr("MOUSE_BUTTON_MIDDLE") = nb::int_(GLFW_MOUSE_BUTTON_3);
    m.attr("JOYSTICK_1") = nb::int_(0);
    m.attr("JOYSTICK_2") = nb::int_(1);
    m.attr("JOYSTICK_3") = nb::int_(2);
    m.attr("JOYSTICK_4") = nb::int_(3);
    m.attr("JOYSTICK_5") = nb::int_(4);
    m.attr("JOYSTICK_6") = nb::int_(5);
    m.attr("JOYSTICK_7") = nb::int_(6);
    m.attr("JOYSTICK_8") = nb::int_(7);
    m.attr("JOYSTICK_9") = nb::int_(8);
    m.attr("JOYSTICK_10") = nb::int_(9);
    m.attr("JOYSTICK_11") = nb::int_(10);
    m.attr("JOYSTICK_12") = nb::int_(11);
    m.attr("JOYSTICK_13") = nb::int_(12);
    m.attr("JOYSTICK_14") = nb::int_(13);
    m.attr("JOYSTICK_15") = nb::int_(14);
    m.attr("JOYSTICK_16") = nb::int_(15);
    m.attr("JOYSTICK_LAST") = nb::int_(GLFW_JOYSTICK_16);
    m.attr("GAMEPAD_BUTTON_A") = nb::int_(0);
    m.attr("GAMEPAD_BUTTON_B") = nb::int_(1);
    m.attr("GAMEPAD_BUTTON_X") = nb::int_(2);
    m.attr("GAMEPAD_BUTTON_Y") = nb::int_(3);
    m.attr("GAMEPAD_BUTTON_LEFT_BUMPER") = nb::int_(4);
    m.attr("GAMEPAD_BUTTON_RIGHT_BUMPER") = nb::int_(5);
    m.attr("GAMEPAD_BUTTON_BACK") = nb::int_(6);
    m.attr("GAMEPAD_BUTTON_START") = nb::int_(7);
    m.attr("GAMEPAD_BUTTON_GUIDE") = nb::int_(8);
    m.attr("GAMEPAD_BUTTON_LEFT_THUMB") = nb::int_(9);
    m.attr("GAMEPAD_BUTTON_RIGHT_THUMB") = nb::int_(10);
    m.attr("GAMEPAD_BUTTON_DPAD_UP") = nb::int_(11);
    m.attr("GAMEPAD_BUTTON_DPAD_RIGHT") = nb::int_(12);
    m.attr("GAMEPAD_BUTTON_DPAD_DOWN") = nb::int_(13);
    m.attr("GAMEPAD_BUTTON_DPAD_LEFT") = nb::int_(14);
    m.attr("GAMEPAD_BUTTON_LAST") = nb::int_(GLFW_GAMEPAD_BUTTON_DPAD_LEFT);
    m.attr("GAMEPAD_BUTTON_CROSS") = nb::int_(GLFW_GAMEPAD_BUTTON_A);
    m.attr("GAMEPAD_BUTTON_CIRCLE") = nb::int_(GLFW_GAMEPAD_BUTTON_B);
    m.attr("GAMEPAD_BUTTON_SQUARE") = nb::int_(GLFW_GAMEPAD_BUTTON_X);
    m.attr("GAMEPAD_BUTTON_TRIANGLE") = nb::int_(GLFW_GAMEPAD_BUTTON_Y);
    m.attr("GAMEPAD_AXIS_LEFT_X") = nb::int_(0);
    m.attr("GAMEPAD_AXIS_LEFT_Y") = nb::int_(1);
    m.attr("GAMEPAD_AXIS_RIGHT_X") = nb::int_(2);
    m.attr("GAMEPAD_AXIS_RIGHT_Y") = nb::int_(3);
    m.attr("GAMEPAD_AXIS_LEFT_TRIGGER") = nb::int_(4);
    m.attr("GAMEPAD_AXIS_RIGHT_TRIGGER") = nb::int_(5);
    m.attr("GAMEPAD_AXIS_LAST") = nb::int_(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER);

    nb::class_<KeyEvent>(m, "KeyEvent")
        .def(nb::init<>())
        .def_rw("key", &KeyEvent::key)
        .def_rw("scancode", &KeyEvent::scancode)
        .def_rw("action", &KeyEvent::action)
        .def_rw("mod", &KeyEvent::mods);

    nb::class_<CharEvent>(m, "CharEvent")
        .def(nb::init<>())
        .def_rw("codepoint", &CharEvent::codepoint);

    nb::class_<CharModsEvent>(m, "CharModsEvent")
        .def(nb::init<>())
        .def_rw("codepoint", &CharModsEvent::codepoint)
        .def_rw("mods", &CharModsEvent::mods);

    nb::class_<MouseButtonEvent>(m, "MouseButtonEvent")
        .def(nb::init<>())
        .def_rw("button", &MouseButtonEvent::button)
        .def_rw("action", &MouseButtonEvent::action)
        .def_rw("mod", &MouseButtonEvent::mods);

    nb::class_<CursorPosEvent>(m, "CursorPosEvent")
        .def(nb::init<>())
        .def_rw("xpos", &CursorPosEvent::xpos)
        .def_rw("ypos", &CursorPosEvent::ypos);

    nb::class_<CursorEnterEvent>(m, "CursorEnterEvent")
        .def(nb::init<>())
        .def_rw("entered", &CursorEnterEvent::entered);

    nb::class_<ScrollEvent>(m, "ScrollEvent")
        .def(nb::init<>())
        .def_rw("xoffset", &ScrollEvent::xoffset)
        .def_rw("yoffset", &ScrollEvent::yoffset);

    nb::class_<DropEvent>(m, "DropEvent")
        .def(nb::init<>())
        .def_ro("count", &DropEvent::paths);

    m.def("get_key", getKey);

    m.def("get_mouse_button", getMouseButton);
    m.def("get_mouse_pos", [&]() {
        return nb::make_tuple(getCursorX(), getCursorY());
    });

    m.def("get_key_events", getKeyEvents);
    m.def("get_char_events", getCharEvents);
    m.def("get_char_mods_events", getCharModsEvents);
    m.def("get_mouse_button_events", getMouseButtonEvents);
    m.def("get_mouse_pos_events", getCursorPosEvents);
    m.def("get_mouse_enter_events", getCursorEnterEvents);
    m.def("get_scroll_events", getScrollEvents);
    m.def("get_drop_events", getDropEvents);

    m.def("is_joystick_present", [](int id) {
        return GLFW_TRUE == glfwJoystickPresent(id);
    });

    m.def("get_joystick_axes", [](int id) {

        int count;
        const float* axes = glfwGetJoystickAxes(id, &count);

        nb::list l;

        for (int i = 0; i < count; ++i) {
            l.append(axes[i]);
        }

        return l;
    });

    m.def("get_joystick_buttons", [](int id) {

        int count;
        const unsigned char* buttons = glfwGetJoystickButtons(id, &count);

        nb::list l;

        for (int i = 0; i < count; ++i) {
            l.append(GLFW_PRESS == buttons[i]);
        }

        return l;
    });

    m.def("get_joystick_name", [](int id) {
        return std::string(glfwGetJoystickName(id));
    });
}

}
