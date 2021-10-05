#include <imgui.h>
#include <ios>
#include <misc/cpp/imgui_stdlib.h>
#include <pybind11/cast.h>
#include <pybind11/detail/common.h>
#include <regex>
#include <functional>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/pytypes.h>
#include <pybind11/pybind11.h>

#include <experimental/filesystem>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <unordered_map>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "implot.h"
#include "imgui_internal.h"

#include "SourceSansPro.h"
#include "input.h"

namespace py = pybind11;

struct ImViz {

    GLFWwindow* window = nullptr;

    bool currentWindowOpen = false;

    bool mod = false;
    bool mod_any = false;

    std::regex re{"(-)?(o|s|d|\\*|\\+)?(r|g|b|y|m|w)?"};

    std::unordered_map<std::string, GLuint> textureCache;

    ImViz () {

        if (!glfwInit()) {
            std::cout << "Could not initialize GLFW!" << std::endl;
            std::exit(-1);
        }

        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);

        window = glfwCreateWindow(
                800,
                600,
                "imviz",
                nullptr,
                nullptr);

        glfwMakeContextCurrent(window);

        glewExperimental = true;

        if (GLEW_OK != glewInit()) {
            std::cout << "GL Extension Wrangler initialization failed!" << std::endl;
            std::exit(-1);
        }

        // input setup
        
        input::registerCallbacks(window);

        // basic imgui setup

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = "./imviz.ini";

        ImPlot::GetStyle().AntiAliasedLines = true;

        // loading font

        io.Fonts->AddFontFromMemoryCompressedTTF(
                getSourceSansProData(),
                getSourceSansProSize(),
                20.0);

        prepareUpdate();
    }

    void prepareUpdate() {

        input::update();

        ImGuiIO& io = ImGui::GetIO();

        if (io.WantCaptureMouse) {
            input::clearMouseInput();
        }

        if (io.WantCaptureKeyboard) {
            input::clearKeyboardInput();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static bool showLightTheme = false;
        static bool showImGuiDemo = false;
        static bool showImPlotDemo = false;

        if (ImGui::BeginMainMenuBar()) {

            if (ImGui::BeginMenu("Show")) {

                ImGui::MenuItem("Light Theme", NULL, &showLightTheme);
                ImGui::MenuItem("ImGui Demo", NULL, &showImGuiDemo);
                ImGui::MenuItem("ImPlot Demo", NULL, &showImPlotDemo);
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (showLightTheme) {
            ImGui::StyleColorsLight();
            ImPlot::StyleColorsLight();
        } else {
            ImGui::StyleColorsDark();
            ImPlot::StyleColorsDark();
        }

        // dock space

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("Main DockSpace", NULL, window_flags);

        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();

        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None
                | ImGuiDockNodeFlags_PassthruCentralNode;

        ImGui::DockSpace(1, ImVec2(0.0f, 0.0f), dockspace_flags);

        ImGui::End();

        if (showImGuiDemo) {
            ImGui::ShowDemoWindow(&showImGuiDemo);
        }

        if (showImPlotDemo) {
            ImPlot::ShowDemoWindow(&showImPlotDemo);
        }
    }

    void doUpdate (bool useVsync) {

        currentWindowOpen = false;

        glfwShowWindow(window);

        ImGui::Render();
        
        // background color taken from the one-and-only tomorrow-night theme

        glClearColor(0.11372549019607843,
                     0.12156862745098039,
                     0.12941176470588237,
                     1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int display_w, display_h;
        glfwMakeContextCurrent(window);
        glfwGetFramebufferSize(window, &display_w, &display_h);

        glViewport(0, 0, display_w, display_h);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwMakeContextCurrent(window);

        glfwSwapInterval(useVsync);
        glfwSwapBuffers(window);
    }

    void trigger () {

        glfwPostEmptyEvent();
    }

    void setMod(bool m) {

        mod = m;
        mod_any |= m;
    }
};

/**
 * Some helpers to make handling arrays easier.
 */

template <typename T>
using array_like = py::array_t<T, py::array::c_style | py::array::forcecast>;

void assertArrayShape(std::string name,
                      py::array& array,
                      std::vector<std::vector<int>> shapes) {

    bool foundShape = false;

    for (auto& shape : shapes) { 
        if ((int)shape.size() != array.ndim()) {
            continue;
        }
        bool ok = true;
        for (size_t i = 0; i < shape.size(); ++i) {
            if (shape[i] == -1) {
                continue;
            }
            ok &= shape[i] == array.shape()[i];
        }
        if (ok) { 
            foundShape = true;
            break;
        }
    }

    if (!foundShape) {
        std::stringstream ss;
        ss << "Expected shape of \"" + name + "\" to fit ";

        for (size_t i = 0; i < shapes.size(); ++i) { 
            ss << "(";
            auto& shape = shapes[i];
            for (size_t k = 0; k < shape.size(); ++k) { 
                ss << shape[k]; 
                if (k != shape.size() - 1) { 
                    ss <<  ", ";
                }
            }
            ss << ")";
            if (i != shapes.size() - 1) { 
                ss << " | ";
            }
        }

        ss << ", but found (";

        for (int k = 0; k < array.ndim(); ++k) { 
            ss << array.shape()[k]; 
            if (k != array.ndim() - 1) { 
                ss <<  ", ";
            }
        }
        ss << ")";

        throw std::runtime_error(ss.str());
    }
}

#define assert_shape(array, ...) assertArrayShape(#array, array, __VA_ARGS__)

ImViz viz;

template<typename T>
ImVec4 interpretColor(T& color) {

    ImVec4 c(0, 0, 0, 1);
    size_t colorLength = color.shape()[0];

    if (colorLength == 1) {
        c.x = color.data()[0];
        c.y = color.data()[0];
        c.z = color.data()[0];
    } else if (colorLength == 3) {
        c.x = color.data()[0];
        c.y = color.data()[1];
        c.z = color.data()[2];
    } else if (colorLength == 4) {
        c.x = color.data()[0];
        c.y = color.data()[1];
        c.z = color.data()[2];
        c.w = color.data()[3];
    } else {
        c = IMPLOT_AUTO_COL;
    }

    return c;
}

struct ImageInfo {

    int imageWidth = 0;
    int imageHeight = 0;
    GLuint textureId = 0;
};

ImageInfo processImage(std::string& id, py::array& image) {

    assert_shape(image, {{-1, -1}, {-1, -1, 1}, {-1, -1, 3}, {-1, -1, 4}});

    // determine image parameters

    int imageWidth = 0;
    int imageHeight = 0;
    int channels = 0;
    GLenum format = 0;
    GLenum datatype = 0;

    if (image.ndim() == 2) {
        imageWidth = image.shape(1);
        imageHeight = image.shape(0);
        channels = 1;
    } else if (image.ndim() == 3) {
        imageWidth = image.shape(1);
        imageHeight = image.shape(0);
        channels = image.shape(2);
    }

    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 3) {
        format = GL_RGB;
    } else if (channels == 4) {
        format = GL_RGBA;
    } 

    if (py::str(image.dtype()).equal(py::str("uint8"))) {
        datatype = GL_UNSIGNED_BYTE;
    } else if (py::str(image.dtype()).equal(py::str("float32"))) {
        datatype = GL_FLOAT;
    } else {
        throw std::runtime_error(
                "Pixel datatype "
                + std::string(py::str(image.dtype()))
                + " not supported. "
                + "Supported datatypes are uint8 and float32.");
    }

    // create a texture if necessary
    if (viz.textureCache.find(id) == viz.textureCache.end()) {

        GLuint textureId;

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);

        viz.textureCache[id] = textureId;
    }

    // upload texture

    GLuint textureId = viz.textureCache[id];

    glBindTexture(GL_TEXTURE_2D, textureId);

    if (format == GL_RED) {
        GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }

    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            format,
            imageWidth,
            imageHeight,
            0,
            format,
            datatype,
            image.data());

    glBindTexture(GL_TEXTURE_2D, 0);

    // return info

    ImageInfo info;
    info.imageWidth = imageWidth;
    info.imageHeight = imageHeight;
    info.textureId = textureId;

    return info;
}

PYBIND11_MODULE(imviz, m) {

    input::loadPythonBindings(m);

    py::enum_<ImGuiCond_>(m, "Cond")
        .value("NONE", ImGuiCond_None)
        .value("ALWAYS", ImGuiCond_Always)
        .value("ONCE", ImGuiCond_Once)
        .value("FIRST_USE_EVER", ImGuiCond_FirstUseEver)
        .value("APPEARING", ImGuiCond_Appearing)
        .export_values();

    m.def("mod", [&]() { return viz.mod; });
    m.def("mod_any", [&]() { return viz.mod_any; });
    m.def("clear_mod_any", [&]() { viz.mod_any = false; });

    m.def("wait", [&](bool vsync) {
        viz.doUpdate(vsync);
        input::update();
        glfwPollEvents();
        viz.prepareUpdate();
        return !glfwWindowShouldClose(viz.window);
    },
    py::arg("vsync") = true);

    m.def("trigger", [&]() {
        viz.trigger();
    });

    m.def("begin_window", [&](std::string label,
                       bool opened,
                       array_like<float> position, 
                       array_like<float> size, 
                       bool title_bar,
                       bool resize,
                       bool move,
                       bool scrollbar,
                       bool scrollWithMouse,
                       bool collapse,
                       bool autoResize) {

        if (position.shape()[0] > 0) { 
            assert_shape(position, {{2}});
            const float* data = position.data();
            ImGui::SetNextWindowPos({data[0], data[1]});
        }
        if (size.shape()[0] > 0) {
            assert_shape(size, {{2}});
            const float* data = size.data();
            ImGui::SetNextWindowSize({data[0], data[1]});
        }

        ImGuiWindowFlags flags = ImGuiWindowFlags_None;

        flags |= ImGuiWindowFlags_NoTitleBar * !title_bar;
        flags |= ImGuiWindowFlags_NoResize * !resize;
        flags |= ImGuiWindowFlags_NoMove * !move;
        flags |= ImGuiWindowFlags_NoScrollbar * !scrollbar;
        flags |= ImGuiWindowFlags_NoScrollWithMouse * !scrollWithMouse;
        flags |= ImGuiWindowFlags_NoCollapse * !collapse;
        flags |= ImGuiWindowFlags_AlwaysAutoResize * autoResize;

        viz.currentWindowOpen = opened;

        bool show = ImGui::Begin(label.c_str(), &viz.currentWindowOpen, flags);

        return show;
    },
    py::arg("label"),
    py::arg("opened") = true,
    py::arg("position") = py::array(),
    py::arg("size") = py::array(),
    py::arg("title_bar") = true,
    py::arg("resize") = true,
    py::arg("move") = true,
    py::arg("scrollbar") = true,
    py::arg("scroll_with_mouse") = true,
    py::arg("collapse") = true,
    py::arg("auto_resize") = false);

    m.def("end_window", ImGui::End);

    m.def("window_open", [&]() { return viz.currentWindowOpen; });

    m.def("begin_plot", [&](std::string label,
                            std::string xLabel,
                            std::string yLabel,
                            bool equalAxis,
                            bool autoFitX,
                            bool autoFitY) {

        ImPlotFlags flags = 0;

        if (equalAxis) {
            flags |= ImPlotFlags_Equal;
        }

        ImPlotAxisFlags xFlags = 0;
        ImPlotAxisFlags yFlags = 0;

        if (autoFitX) { 
            xFlags |= ImPlotAxisFlags_AutoFit;
        }

        if (autoFitY) { 
            yFlags |= ImPlotAxisFlags_AutoFit;
        }

        return ImPlot::BeginPlot(label.c_str(),
                      xLabel.c_str(),
                      yLabel.c_str(),
                      ImGui::GetContentRegionAvail(),
                      flags,
                      xFlags,
                      yFlags);
    },
    py::arg("label") = "",
    py::arg("x_label") = "",
    py::arg("y_label") = "",
    py::arg("equal_axis") = false,
    py::arg("auto_fit_x") = false,
    py::arg("auto_fit_y") = false);

    m.def("end_plot", &ImPlot::EndPlot);

    m.def("tree_node", [&](std::string label) {

        return ImGui::TreeNode(label.c_str());
    },
    py::arg("label") = "");

    m.def("tree_pop", ImGui::TreePop);

    m.def("begin_menu_bar", ImGui::BeginMenuBar);
    m.def("end_menu_bar", ImGui::EndMenuBar);

    m.def("begin_main_menu_bar", ImGui::BeginMainMenuBar);
    m.def("end_main_menu_bar", ImGui::EndMainMenuBar);

    m.def("begin_menu", [&](std::string label, bool enabled) {

        return ImGui::BeginMenu(label.c_str(), enabled);
    },
    py::arg("label"),
    py::arg("enabled") = true);

    m.def("end_menu", ImGui::EndMenu);

    m.def("menu_item", [&](std::string label, std::string shortcut, bool selected, bool enabled) {
        
        return ImGui::MenuItem(label.c_str(), shortcut.c_str(), selected, enabled);
    },
    py::arg("label"),
    py::arg("shortcut") = "",
    py::arg("selected") = false,
    py::arg("enabled") = true);

    m.def("button", [&](std::string label) {

        return ImGui::Button(label.c_str());
    },
    py::arg("label"));

    m.def("combo", [&](std::string label, py::list items, py::handle selection) {

        size_t len = items.size();

        std::vector<std::string> objStr(len);
        std::vector<const char*> objPtr(len);

        int selectionIndex = 0;

        int i = 0;
        for (const py::handle& o : items) {

            objStr[i] = py::str(o);
            objPtr[i] = objStr[i].c_str();

            if (o.equal(selection)) {
                selectionIndex = i;
            }

            i += 1;
        }

        bool mod = ImGui::Combo(label.c_str(), &selectionIndex, objPtr.data(), len);
        viz.setMod(mod);

        return items[selectionIndex];
    },
    py::arg("label"),
    py::arg("items"),
    py::arg("selection") = py::none());

    m.def("text", [&](std::string str, array_like<double> color) {

        ImVec4 c = interpretColor(color);

        if (c.w >= 0) {
            ImGui::TextColored(c, "%s", str.c_str());
        } else {
            ImGui::Text("%s", str.c_str());
        }
    },
    py::arg("str"),
    py::arg("color") = py::array_t<double>());

    m.def("input", [&](std::string label, std::string& obj) {
        
        bool mod = ImGui::InputText(label.c_str(), &obj);
        viz.setMod(mod);

        return obj;
    }, 
    py::arg("label"),
    py::arg("obj"));

    m.def("input", [&](std::string label, int& obj) {
        
        bool mod = ImGui::InputInt(label.c_str(), &obj);
        viz.setMod(mod);

        return obj;
    }, 
    py::arg("label"),
    py::arg("obj"));

    m.def("input", [&](std::string label, double& obj) {
        
        bool mod = ImGui::InputDouble(label.c_str(), &obj);
        viz.setMod(mod);

        return obj;
    }, 
    py::arg("label"),
    py::arg("obj"));

    m.def("checkbox", [&](std::string label, bool& obj) {
        
        bool mod = ImGui::Checkbox(label.c_str(), &obj);
        viz.setMod(mod);

        return obj;
    }, 
    py::arg("label"),
    py::arg("obj"));

    m.def("slider", [&](std::string title, double& value, double min, double max) {
        
        bool mod = ImGui::SliderScalar(
                title.c_str(), ImGuiDataType_Double, &value, &min, &max);
        viz.setMod(mod);

        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
    py::arg("min") = 0.0,
    py::arg("max") = 1.0);

    m.def("drag", [&](std::string title, int& value, float speed, int min, int max) {
        
        bool mod = ImGui::DragInt(title.c_str(), &value, speed, min, max);
        viz.setMod(mod);

        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
    py::arg("speed") = 1.0,
    py::arg("min") = 0.0,
    py::arg("max") = 0.0);

    m.def("drag", [&](std::string title, double& value, float speed, double min, double max) {
        
        bool mod = ImGui::DragScalar(title.c_str(),
                ImGuiDataType_Double, &value, speed, &min, &max);
        viz.setMod(mod);

        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
    py::arg("speed") = 0.1,
    py::arg("min") = 0.0,
    py::arg("max") = 0.0);

    m.def("range", [&](std::string label, py::tuple range, float speed, float min, float max) {

        float minVal = py::cast<float>(range[0]);
        float maxVal = py::cast<float>(range[1]);
        
        bool mod = ImGui::DragFloatRange2(label.c_str(), &minVal, &maxVal, speed, min, max);

        if (ImGui::BeginPopupContextItem(label.c_str())) {

            if (ImGui::MenuItem("Expand range")) {
                minVal = min;
                maxVal = max;
            }
            if (ImGui::MenuItem("Expand to min")) {
                minVal = min;
            }
            if (ImGui::MenuItem("Expand to max")) {
                maxVal = max;
            }
            if (ImGui::MenuItem("Collapse to min")) {
                maxVal = minVal;
            }
            if (ImGui::MenuItem("Collapse to max")) {
                minVal = maxVal;
            }

            ImGui::EndPopup();
        }

        viz.setMod(mod);

        return py::make_tuple(minVal, maxVal);
    }, 
    py::arg("label"),
    py::arg("range"),
    py::arg("speed") = 1.0f,
    py::arg("min") = 0,
    py::arg("max") = 0);

    m.def("color_edit", [&](std::string label, array_like<float> color) {

        if (color.ndim() != 1) {
            throw std::runtime_error("Color must have 1 dimension but has "
                    + std::to_string(color.ndim()) + ".");
        }

        bool mod = false;

        size_t colorSize = color.shape()[0];
        if (colorSize == 3) {
            mod = ImGui::ColorEdit3(label.c_str(), color.mutable_data());
        } else if (colorSize == 4) {
            mod = ImGui::ColorEdit4(label.c_str(), color.mutable_data());
        } else {
            throw std::runtime_error(
                    "Color must have 3 or 4 elements but has "
                    + std::to_string(colorSize) + ".");
        }

        viz.setMod(mod);

        return color;
    });

    m.def("same_line", []() {
        ImGui::SameLine();
    });

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

    m.def("image", [&](
                std::string id,
                py::array& image,
                int displayWidth,
                int displayHeight) {

        ImageInfo info = processImage(id, image);

        if (displayWidth < 0) {
            displayWidth = info.imageWidth;
        }
        if (displayHeight < 0) {
            displayHeight = info.imageHeight;
        }

        ImGui::Image(
                (void*)(intptr_t)info.textureId,
                ImVec2(displayWidth, displayHeight));
    },
    py::arg("id"),
    py::arg("image"),
    py::arg("width") = -1,
    py::arg("height") = -1);

    m.def("plot_image", [&](
                std::string id,
                py::array& image,
                double x,
                double y,
                double displayWidth,
                double displayHeight) {

        ImageInfo info = processImage(id, image);

        // display texture

        if (displayWidth < 0) {
            displayWidth = info.imageWidth;
        }
        if (displayHeight < 0) {
            displayHeight = info.imageHeight;
        }

        ImPlotPoint boundsMin(x, y);
        ImPlotPoint boundsMax(x + displayWidth, y + displayHeight);

        ImPlot::PlotImage(
                id.c_str(),
                (void*)(intptr_t)info.textureId,
                boundsMin,
                boundsMax);
    },
    py::arg("id"),
    py::arg("image"),
    py::arg("x") = 0,
    py::arg("y") = 0,
    py::arg("width") = -1,
    py::arg("height") = -1);

    m.def("next_plot_limits", [&](
                double xmin,
                double xmax,
                double ymin,
                double ymax,
                ImGuiCond_ cond) {

        ImPlot::SetNextPlotLimits(xmin, xmax, ymin, ymax, cond);
    },
    py::arg("xmin"),
    py::arg("xmax"),
    py::arg("ymin"),
    py::arg("ymax"),
    py::arg("cond") = ImGuiCond_Once);

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

    m.def("begin_tab_bar", [&](std::string& name) {
        return ImGui::BeginTabBar(name.c_str());
    });
    m.def("end_tab_bar", ImGui::EndTabBar);

    m.def("begin_tab_item", [&](std::string& name) {
        return ImGui::BeginTabItem(name.c_str());
    });
    m.def("end_tab_item", ImGui::EndTabItem);

    m.def("plot", [&](array_like<double> x,
                      array_like<double> y,
                      std::string fmt,
                      std::string label,
                      array_like<double> shade,
                      float shadeAlpha,
                      float lineWeight,
                      float markerSize,
                      float markerWeight) {

        std::smatch match;
        std::regex_search(fmt, match, viz.re);

        std::vector<std::string> groups;
        for (auto m : match) {
            if (m.matched) {
                groups.push_back(m.str());
            } else {
                groups.push_back("");
            }
        }

        ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, lineWeight);
        ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, markerSize);
        ImPlot::PushStyleVar(ImPlotStyleVar_MarkerWeight, markerWeight);

        if (groups[2] == "o") {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Circle);
        } else if (groups[2] == "s") {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Square);
        } else if (groups[2] == "d") {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Diamond);
        } else if (groups[2] == "+") {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Cross);
        } else if (groups[2] == "*") {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Asterisk);
        } else {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_None);
        }

        size_t yCount = y.shape()[0];

        std::vector<double> indices;
        const double* xDataPtr = nullptr;
        const double* yDataPtr = nullptr;
        size_t count = 0;

        if (1 == x.ndim() && 0 == yCount) {
            count = x.shape()[0];
            indices.resize(count);
            for (size_t i = 0; i < count; ++i) {
                indices[i] = i;
            }
            xDataPtr = indices.data();
            yDataPtr = x.data();
        } else if (2 == x.ndim() && 0 == yCount) {
            size_t len0 = x.shape()[0];
            size_t len1 = x.shape()[1];
            if (len0 == 2) {
                xDataPtr = x.data();
                yDataPtr = x.data() + len1;
                count = len1;
            }
        } else if (1 == x.ndim() && 1 == y.ndim()) {
            count = std::min(x.shape()[0], y.shape()[0]);
            xDataPtr = x.data();
            yDataPtr = y.data();
        } 

        if (count == 0) {
            throw std::runtime_error("Data format could not be interpreted");
        }

        if (groups[1] == "-") {
            ImPlot::PlotLine(label.c_str(), xDataPtr, yDataPtr, count);
        } else {
            ImPlot::PlotScatter(label.c_str(), xDataPtr, yDataPtr, count);
        }

        size_t shadeCount = std::min(count, (size_t)shade.shape()[0]);

        if (shadeCount != 0) {
            if (1 == shade.ndim()) {
                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, shadeAlpha);
                auto mean = py::cast<py::array_t<double>>(y[py::slice(0, shadeCount, 1)]);
                py::array_t<double> upper = mean + shade;
                py::array_t<double> lower = mean - shade;
                ImPlot::PlotShaded(label.c_str(), xDataPtr, lower.data(), upper.data(), shadeCount);
                ImPlot::PopStyleVar();
            }
        }

        ImPlot::PopStyleVar(4);
    },
    py::arg("x"),
    py::arg("y") = py::array(),
    py::arg("fmt") = "-",
    py::arg("label") = "",
    py::arg("shade") = py::array(),
    py::arg("shade_alpha") = 0.3f,
    py::arg("line_weight") = 1.0f, 
    py::arg("marker_size") = 4.0f, 
    py::arg("marker_weight") = 1.0f);

    m.def("drag_point", [&](std::string label,
                            array_like<double> point,
                            bool showLabel,
                            array_like<double> color,
                            double radius) {

        double x = point.data()[0];
        double y = point.data()[1];

        ImVec4 c = interpretColor(color);

        bool mod = ImPlot::DragPoint(label.c_str(), &x, &y, showLabel, c, radius);
        viz.setMod(mod);

        return py::make_tuple(x, y);
    },
    py::arg("label"),
    py::arg("point"),
    py::arg("show_label") = false,
    py::arg("color") = py::array_t<double>(),
    py::arg("radius") = 4.0);

    m.def("drag_vline", [&](std::string label,
                            double x,
                            bool showLabel,
                            array_like<double> color,
                            double width) {

        ImVec4 c = interpretColor(color);

        bool mod = ImPlot::DragLineX(label.c_str(), &x, showLabel, c, width);
        viz.setMod(mod);

        return x;
    },
    py::arg("label"),
    py::arg("x"),
    py::arg("show_label") = false,
    py::arg("color") = py::array_t<double>(),
    py::arg("width") = 1.0);

    m.def("drag_hline", [&](std::string label,
                            double y,
                            bool showLabel,
                            array_like<double> color,
                            double width) {

        ImVec4 c = interpretColor(color);

        bool mod = ImPlot::DragLineY(label.c_str(), &y, showLabel, c, width);
        viz.setMod(mod);

        return y;
    },
    py::arg("label"),
    py::arg("y"),
    py::arg("show_label") = false,
    py::arg("color") = py::array_t<double>(),
    py::arg("width") = 1.0);

    m.def("get_plot_pos", [&]() {

        ImVec2 plotPos = ImPlot::GetPlotPos();
        py::array_t<double> pos(2);
        pos.mutable_at(0) = plotPos.x;
        pos.mutable_at(1) = plotPos.y;

        return pos;
    });

    m.def("get_plot_size", [&]() {

        ImVec2 plotSize = ImPlot::GetPlotSize();

        py::array_t<double> pos(2);
        pos.mutable_at(0) = plotSize.x;
        pos.mutable_at(1) = plotSize.y;

        return pos;
    });

    m.def("annotate", [&](
                double x,
                double y,
                std::string text,
                array_like<double> color,
                array_like<double> offset,
                bool clamp) {

        ImVec4 col = interpretColor(color);
        if (col.w < 0) {
            col = ImVec4(1.0, 1.0, 1.0, 0.0);
        }

        ImVec2 o;

        if (offset.shape(0) > 0) {
            assert_shape(offset, {{2}});
            o.x = offset.data()[0];
            o.y = offset.data()[1];
        }

        if (clamp) {
            ImPlot::AnnotateClamped(x, y, o, col, "%s", text.c_str());
        } else {
            ImPlot::Annotate(x, y, o, col, "%s", text.c_str());
        }
    },
    py::arg("x"),
    py::arg("y"),
    py::arg("text"),
    py::arg("color") = py::array(),
    py::arg("offset") = py::array(),
    py::arg("clamp") = false);

    m.def("begin_popup_context_item", [&](std::string label) {
        if (label.empty()) {
            return ImGui::BeginPopupContextItem();
        } else {
            return ImGui::BeginPopupContextItem(label.c_str());
        }
    },
    py::arg("label") = "");

    m.def("begin_popup", [&](std::string label) {
        return ImGui::BeginPopup(label.c_str());
    });

    m.def("end_popup", ImGui::EndPopup);

    m.def("activate_svg", [&]() {

        //ImDrawList::svg = new std::stringstream();
    });

    m.def("get_svg", [&]() {

        /*
        std::stringstream* svg = ImDrawList::svg;
        std::string result = svg->str();

        delete svg;
        ImDrawList::svg = nullptr;

        return result;
        */
    });
}
