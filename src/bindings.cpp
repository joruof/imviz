#include <iostream>
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
#include "shader_program.hpp"

/**
 * This allows us to handle imgui assertions via exceptions on the python side.
 */
void checkAssertion(bool expr, const char* exprStr) {

    if (not expr) { 
        throw std::runtime_error(exprStr);
    }
}

namespace py = pybind11;

ImViz viz;

PYBIND11_MODULE(cppimviz, m) {

    /**
     * Input module bindings
     */

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

    m.def("set_main_window_icon", [&](array_like<uint8_t> arr) {

        assert_shape(arr, {{-1, -1, 4}});

        GLFWimage img;
        img.height = arr.shape(0);
        img.width = arr.shape(1);
        img.pixels = arr.mutable_data();

        glfwSetWindowIcon(viz.window, 1, &img);
    });

    m.def("get_clipboard", [&]() { 

        const char* str = glfwGetClipboardString(NULL);

        if (nullptr == str) { 
            return std::string("");
        } else {
            return std::string(str);
        }
    });

    m.def("set_clipboard", [&](std::string str) { 

        glfwSetClipboardString(NULL, str.c_str());
    });

    /**
     * Custom widgets
     */

    m.def("file_dialog_popup", [&](
                std::string label,
                std::string path,
                std::string confirmText) {

        bool mod = ImGui::FileDialogPopup(
                label.c_str(), confirmText.c_str(), path);
        viz.setMod(mod);

        return path;
    },
    py::arg("label"),
    py::arg("path"),
    py::arg("confirmText") = "Ok");

    m.def("multiselect", [&](
                std::string label,
                py::list& values,
                py::list selection) {

        bool mod = false;

        if (ImGui::BeginPopup(label.c_str())) {

            for (py::handle o : values) {
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
    py::arg("label"),
    py::arg("values"),
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

            for (ssize_t r = 0; r < index.size(); ++r) {
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
    });

    m.def("set_mod", [&](bool m) {
        viz.setMod(m);
    });

    m.def("mod_any", [&]() {
        return viz.mod_any;
    });

    m.def("clear_mod_any", [&]() { 
        viz.mod_any = false;
    });

    m.def("trigger", [&]() {
        viz.trigger();
    });

    m.def("wait", [&](bool vsync, bool powersave, double timeout) {

        try {
            viz.doUpdate(vsync);
        } catch (std::runtime_error& e) { 
            // last resort: if we catch an error here, all we can do
            // is create the context from scratch and hope for the best.

            std::cerr << e.what() << std::endl;

            viz.setupImLibs();
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
    py::arg("vsync") = true,
    py::arg("powersave") = false,
    py::arg("timeout") = 1.0);

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

        py::array_t<uint8_t> pixels({width, height, 4});

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

        return pixels[py::slice(height, 0, -1)].attr("copy")();
    });

    /**
     * Simple renderer
     */

    py::class_<ShaderProgram>(m, "ShaderProgram")
        .def(py::init<std::string, std::string>())
        .def("get_uniforms", [](ShaderProgram& p) { 
            glUseProgram(p.id);

            GLint count;
            glGetProgramiv(p.id, GL_ACTIVE_UNIFORMS, &count);

            std::cout << count << std::endl;

            for (GLint i = 0; i < count; i++)
            {
                GLint size; 
                GLenum type; 
                const GLsizei bufSize = 128; 
                GLchar name[bufSize]; 
                GLsizei length; 

                glGetActiveUniform(p.id, (GLuint)i, bufSize, &length, &size, &type, name);
                printf("Uniform #%d Type: %u Name: %s Size: %d\n", i, type, name, size);
            }
        })
        .def("get_attributes", [](ShaderProgram& p) { 

            glUseProgram(p.id);

            GLint count;
            glGetProgramiv(p.id, GL_ACTIVE_ATTRIBUTES, &count);

            std::cout << count << std::endl;

            for (GLint i = 0; i < count; i++)
            {
                GLint size; 
                GLenum type; 
                const GLsizei bufSize = 128; 
                GLchar name[bufSize]; 
                GLsizei length; 

                glGetActiveAttrib(p.id, (GLuint)i, bufSize, &length, &size, &type, name);
                printf("Uniform #%d Type: %u Name: %s Size: %d\n", i, type, name, size);
            }
        });
}
