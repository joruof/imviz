#include <imgui.h>
#include <iostream>
#include <pybind11/cast.h>
#include <pybind11/numpy.h>
#include <pybind11/pytypes.h>
#include <stdexcept>

#include <GL/glew.h>

#include "implot.h"
#include "implot_internal.h"

#include "im_user_config.h"

#include "imviz.hpp"
#include "input.hpp"
#include "file_dialog.hpp"
#include "binding_helpers.hpp"
#include "bindings_implot.hpp"
#include "bindings_imgui.hpp"
#include "load_image.hpp"

/**
 * This allows us to handle imgui assertions via exceptions on the python side.
 */
void checkAssertion(bool expr, const char* exprStr) {
    if (!expr) { 
        throw std::runtime_error(exprStr);
    }
}

namespace py = pybind11;

ImViz viz;

PYBIND11_MODULE(cppimviz, m) {

    /**
     * Input module bindings
     */

    viz.init();
    input::loadPythonBindings(m);

    loadImguiPythonBindings(m, viz);
    loadImplotPythonBindings(m, viz);

    /**
     * GLFW functions
     */

    m.def("set_main_window_title", [&](std::string title) {
        glfwSetWindowTitle(viz.window, title.c_str());
    },
    py::arg("title"));

    m.def("set_main_window_size", [&](ImVec2 size) {
        glfwSetWindowSize(viz.window, size.x, size.y);
    },
    py::arg("size"));

    m.def("enter_fullscreen", [&]() {

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwSetWindowMonitor(viz.window, monitor,
                             0, 0,
                             mode->width, mode->height,
                             mode->refreshRate);
    });

    m.def("leave_fullscreen", [&]() {
        glfwSetWindowMonitor(viz.window, nullptr, 0, 0, 800, 600, 0);
    });

    m.def("get_main_window_size", [&]() {
        int w, h;
        glfwGetWindowSize(viz.window, &w, &h);
        return ImVec2(w, h);
    });

    m.def("set_main_window_pos", [&](ImVec2 pos) {
        glfwSetWindowPos(viz.window, pos.x, pos.y);
    },
    py::arg("size"));

    m.def("get_main_window_pos", [&]() {
        int x, y;
        glfwGetWindowPos(viz.window, &x, &y);
        return ImVec2(x, y);
    });

    m.def("get_main_window_size", [&]() {
        int w, h;
        glfwGetWindowSize(viz.window, &w, &h);
        return ImVec2(w, h);
    });

    m.def("hide_main_window", [&]() {
        glfwHideWindow(viz.window);
    });

    m.def("show_main_window", [&]() {
        glfwShowWindow(viz.window);
    });

    m.def("set_main_window_icon", [&](array_like<uint8_t> icon) {

        assert_shape(icon, {{-1, -1, 4}});

        GLFWimage img;
        img.height = icon.shape(0);
        img.width = icon.shape(1);
        img.pixels = icon.mutable_data();

        glfwSetWindowIcon(viz.window, 1, &img);
    },
    R"raw(
    This sets the icon of the main window shown in e.g. taskbars.
    The *icon* must be an RGBA image given as an array_like with shape (-1, -1, 4).
    )raw", 
    py::arg("icon"));

    m.def("get_clipboard", [&]() { 

        const char* str = glfwGetClipboardString(NULL);

        if (nullptr == str) { 
            return std::string("");
        } else {
            return std::string(str);
        }
    },
    R"raw(
    Returns a string from the system clipboard.
    )raw"
    );

    m.def("set_clipboard", [&](std::string str) { 

        glfwSetClipboardString(NULL, str.c_str());
    });

    /**
     * Custom widgets
     */

    m.def("file_dialog_popup", [&](
                std::string label,
                std::string path,
                std::string confirm_text) {

        bool mod = ImGui::FileDialogPopup(
                label.c_str(), confirm_text.c_str(), path);
        viz.setMod(mod);

        return path;
    },
    R"raw(
    Creates a simple file chooser popup window with the given *label* as id.
    To show the dialog it must be opened using "imviz.open_popup(*label*)".
    The starting directory will be the given *path*.
    The confirm button can be customized with the *confirm_text* argument.

    Returns the path of the newly selected element.
    )raw",
    py::arg("label"),
    py::arg("path"),
    py::arg("confirm_text") = "Ok");

    m.def("multiselect", [&](
                std::string label,
                py::list& options,
                py::list selection) {

        bool mod = false;

        if (ImGui::BeginPopup(label.c_str())) {

            for (py::handle o : options) {
                std::string ostr = py::str(o);

                bool inList = selection.contains(o);

                // this will be modified by the checkbox
                bool selected = inList;

                if (ImGui::Checkbox(ostr.c_str(), &selected)) {
                    if (selected && !inList) {
                        selection.append(o);
                    } else if (!selected && inList) {
                        selection.attr("remove")(o);
                    }

                    mod = true;
                }
            }

            ImGui::EndPopup();
        }

        if (ImGui::Button(label.c_str())) {
            ImGui::OpenPopup(label.c_str());
        }

        viz.setMod(mod);

        return selection;
    },
    R"raw(
    Creates a button, which (on click) allows the selection of multiple options.
    The *options* must be given as a python list of objects.
    Each option will be converted via "str(...)" to a string to be rendered.
    Options contained in the *selection* list will be rendered as selected.

    Returns the new (possibly modified) selection of options as a list.
    )raw",
    py::arg("label"),
    py::arg("options"),
    py::arg("selection"));

    m.def("dataframe", [&](
                py::object frame,
                std::string label,
                py::list selection) {

        py::list keys = frame.attr("keys")();
        py::function itemFunc = frame.attr("__getitem__");
        py::array index = frame.attr("index");
        py::function indexGetFunc = index.attr("__getitem__");

        size_t columnCount = 2 + py::len(keys);

        ImGuiTableFlags flags =
            ImGuiTableFlags_Borders
            | ImGuiTableFlags_RowBg
            | ImGuiTableFlags_Resizable
            | ImGuiTableFlags_Reorderable
            | ImGuiTableFlags_ScrollX
            | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable(label.c_str(), columnCount, flags)) {

            std::vector<py::function> getColDataFuncs;
            getColDataFuncs.push_back(indexGetFunc);

            ImGui::TableSetupColumn("");
            ImGui::TableSetupColumn("index");

            for (const py::handle& o : keys) {

                std::string key = py::cast<std::string>(o);

                getColDataFuncs.push_back(
                        itemFunc(key)
                        .attr("astype")("str")
                        .attr("__getitem__"));

                ImGui::TableSetupColumn(key.c_str());
            }

            ImGui::TableHeadersRow();

            for (size_t r = 0; r < index.size(); ++r) {
                ImGui::TableNextRow();

                py::object rowIndex = indexGetFunc(r);
                bool state = selection.contains(rowIndex);

                ImGui::TableSetColumnIndex(0);

                ImGui::PushID(r);
                
                if (ImGui::Checkbox("###marked", &state)) {
                    if (state) {
                        selection.append(rowIndex);
                    } else if (selection.contains(rowIndex)) {
                        selection.attr("remove")(rowIndex);
                    }
                }

                ImGui::PopID();

                for (size_t c = 0; c < getColDataFuncs.size(); ++c) {
                    ImGui::TableSetColumnIndex(c + 1);
                    std::string txt{py::str(getColDataFuncs[c](r))};
                    ImGui::Text("%s", txt.c_str());
                }
            }

            ImGui::EndTable();
        }

        return selection;
    },
    py::arg("frame"),
    py::arg("label") = "",
    py::arg("selection") = py::list{});

    /*
     * Essential custom functions
     */

    m.def("mod", [&]() {
        return viz.mod;
    },
    R"raw(
    In C++ most ImGui functions return their modification status as boolean,
    while the actual data values are passed and modified by reference.

    Because python does not allow pass-by-reference for primitive datatypes,
    this behavior cannot be replicated exactly in python.

    In imviz all functions instead return the modified data directly.
    Whether data was modified by can be queried by using this function.
    
    It returns the modification status as set by the previous call.
    )raw"
    );

    m.def("set_mod", [&](bool m) {
        viz.setMod(m);
    },
    R"raw(
    Can be used to overwrite the modification status flag.
    )raw",
    py::arg("mod")
    );

    m.def("mod_any", [&]() {
        bool m = viz.mod_any.back();
        return m;
    },
    R"raw(
    In contrast to the mod flag, the mod_any flag retuns the | ("or")
    combination of the modification status flags of all previous calls.
    Can be used to check, if data was modified by any function call in
    a group of calls.
    )raw"
    );

    m.def("clear_mod_any", [&]() {
        viz.mod_any.back() = false;
    },
    R"raw(
    Resets the mod_any flag to False.
    )raw"
    );

    m.def("push_mod_any", [&]() {
        viz.mod_any.push_back(false);
    },
    R"raw(
    Pushes a cleared mod_any flag (False) to the mod_any stack.
    )raw"
    );

    m.def("pop_mod_any", [&]() {
        bool lastMod = viz.mod_any.back();
        if (viz.mod_any.size() > 1) {
            viz.mod_any.pop_back();
            viz.mod_any.back() = lastMod | viz.mod_any.back();
        }
        return lastMod;
    },
    R"raw(
    Pops and returns the mod_any flag at the top of the mod_any stack.
    )raw"
    );

    m.def("trigger", [&]() {
        viz.trigger();
    });

    m.def("wait", [&](bool vsync, bool powersave, double timeout) {

        resetDragDrop();

        // release the gil here so that other threads
        // may do something valueable while we wait 
        py::gil_scoped_release release;

        try {
            viz.doUpdate(vsync);
        } catch (std::runtime_error& e) { 
            // last resort: if we catch an error here soft recovery failed
            // recreate the context from scratch and hope for the best

            std::cerr << e.what() << std::endl;

            viz.setupImLibs();

            // reconfigure and load ini
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = viz.iniFilePath.c_str();
            ImGui::LoadIniSettingsFromDisk(io.IniFilename);

            viz.prepareUpdate();

            return !glfwWindowShouldClose(viz.window);
        }

        input::update();

        if (powersave) {
            if (viz.powerSaveFrameCounter > 0) {
                glfwPollEvents();
                viz.powerSaveFrameCounter -= 1;
            } else {
                glfwWaitEventsTimeout(timeout);
                viz.powerSaveFrameCounter = 5;
            }
        } else {
            glfwPollEvents();
        }

        viz.prepareUpdate();
        return !glfwWindowShouldClose(viz.window);
    },
    R"raw(
    This is the main update function of imviz.
    It must be called once for each frame/update/tick of the application.

    It is typically called as the condition for the main while loop of
    the application. Like so:

    ```
    while viz.wait(...):
        update_app()
    ```

    It returns False if the closing of the main application window was
    requested, and True otherwise.

    If *vsync* is True ```imviz.wait()``` will wait, to synchronize with
    the monitor update rate.

    If *powersave* is True the function will wait for max. *timeout* seconds,
    if NO user input was detected. Otherwise it will return immediately.
    )raw",
    py::arg("vsync") = true,
    py::arg("powersave") = false,
    py::arg("timeout") = 1.0);

    /**
     * Image loading
     */

    m.def("load_image", [](std::string path, int forceChannels) {

        int x = 0;
        int y = 0;
        int n = 0;

        unsigned char* data = loadImage(path.c_str(), &x, &y, &n, forceChannels);
        if (data == nullptr) {
            return py::object(py::none()).release();
        }
        py::array_t<unsigned char> img({y, x, n}, data);
        free(data);

        return py::object(img).release();
    },
    R"raw(
    Uses stb_image.h to load an image from *path* as a numpy array.
    Supported are format: JPEG, PNG, TGA, BMP, PSD, GIF, HDR, PIC, PNM

    Returns the image as numpy array or None, if loading failed.

    The *channels* argument can be used to force a certain channel count.
    )raw",
    py::arg("path"),
    py::arg("channels") = 0);

    /**
     * SVG export
     */

    m.def("begin_svg", [&]() {

        ImDrawList::svg = new std::stringstream();
        ImDrawList::svgMaxX = -2147483648;
        ImDrawList::svgMaxY = -2147483648;
        ImDrawList::svgMinX = 2147483647;
        ImDrawList::svgMinY = 2147483647;
    });

    m.def("end_svg", [&]() {

        std::stringstream* svg = ImDrawList::svg;

        if (svg == nullptr) {
            return std::string("");
        }

        std::string result = svg->str();

        int svgWidth = ImDrawList::svgMaxX - ImDrawList::svgMinX;
        int svgHeight = ImDrawList::svgMaxY - ImDrawList::svgMinY;

        std::stringstream txt;

        txt << "<svg viewBox=\"" 
            << ImDrawList::svgMinX << " " 
            << ImDrawList::svgMinY << " " 
            << svgWidth << " "
            << svgHeight << " " 
            << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";

        txt << "<rect x=\"" << ImDrawList::svgMinX
            << "\" y=\"" << ImDrawList::svgMinY
            << "\" width=\"" << svgWidth
            << "\" height=\"" << svgHeight 
            << "\" fill=\"";

        ImU32 col = ImGui::GetColorU32(ImGuiCol_WindowBg);

        svgColor(col, txt);
        txt << "\" ";
        svgOpacity(col, txt);
        txt << "/>\n";

        txt << result << "</svg>";

        delete svg;
        ImDrawList::svg = nullptr;

        return txt.str();
    });

    /**
     * Image export
     */

    m.def("get_pixels", [&](int x, int y, int width, int height) {

        py::array_t<uint8_t> pixels({height, width, 4});

        int w, h;
        glfwGetWindowSize(viz.window, &w, &h);

        glReadPixels(
                x, h - y - height,
                width, height,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                pixels.mutable_data());

        // y-axis is flipped when loading directly from gpu
        // need to flip back and copy to fix memory layout

        return pixels[py::slice(-1, -height-1, -1)].attr("copy")();
    },
    R"raw(
    Cuts and returns the specified region from the main framebuffer of 
    the application window.

    The region is specified in window coordinates, starting at the top
    left corner of the window.

    The region will be returned as uint8-RGBA numpy array
    with shape (height, width, 4).
    )raw",
    py::arg("x") = 0,
    py::arg("y") = 0,
    py::arg("width") = -1,
    py::arg("height") = -1);

    m.def("get_texture", [&](GLuint textureId) {

        glBindTexture(GL_TEXTURE_2D, textureId);

        GLint w;
        GLint h;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

        GLint internalFormat;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
                                 GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);

        GLint d = 0;
        if (internalFormat == GL_RED) {
            d = 1;
        } else if (internalFormat == GL_RGB) {
            d = 3;
        } else if (internalFormat == GL_RGBA) {
            d = 4;
        } else {
            throw std::runtime_error("Unknown internal texture format!");
        }

        py::array_t<uint8_t> pixels({w, h, d});

        glGetTexImage(GL_TEXTURE_2D,
                      0,
                      internalFormat,
                      GL_UNSIGNED_BYTE,
                      (void*)pixels.mutable_data(0));

        return pixels;
    },
    R"raw(
    Cuts and returns the specified region from the main framebuffer of 
    the application window.

    The region is specified in window coordinates, starting at the top
    left corner of the window.

    The region will be returned as uint8-RGBA numpy array
    with shape (height, width, 4).
    )raw",
    py::arg("texture_id") = 0);
}
