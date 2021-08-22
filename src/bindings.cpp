#include <imgui.h>
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
#include <stdexcept>
#include <unordered_map>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "implot.h"

#include "SourceSansPro.h"

namespace py = pybind11;

struct ImViz {

    GLFWwindow* window = nullptr;

    bool hasCurrentFigure = false;
    bool currentFigureOpen = false;

    size_t figureCounter = 1;

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

        // basic imgui setup

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();

        ImGui_ImplGlfw_InitForOpenGL(window, false);
        ImGui_ImplOpenGL3_Init("#version 330");

        glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
        glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
        glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
        glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);

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

        ImGuiIO& io = ImGui::GetIO();

        if (io.WantCaptureMouse) {
            // do something ... 
        }
        if (io.WantCaptureKeyboard) {
            // do something ... 
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

        if (hasCurrentFigure) {
            if (currentFigureOpen) {
                ImPlot::EndPlot();
            }
            ImGui::End();
        }

        hasCurrentFigure = false;
        currentFigureOpen = false;

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

        figureCounter = 0;
    }

    void trigger () {

        glfwPostEmptyEvent();
    }
};

ImViz viz;

PYBIND11_MODULE(imviz, m) {

    m.def("wait", [&](bool vsync) {
        viz.doUpdate(vsync);
        glfwPollEvents();
        viz.prepareUpdate();
        return !glfwWindowShouldClose(viz.window);
    },
    py::arg("vsync") = true);

    /*
     * This needs some work until functional.
     *
    m.def("show", [&]() {
        while(!glfwWindowShouldClose(window)) {
            doUpdate();
            glfwWaitEvents();
            prepareUpdate();
        }
    });
    */

    m.def("trigger", [&]() {
        viz.trigger();
    });

    m.def("figure", [&](std::string title,
                        std::string xLabel,
                        std::string yLabel,
                        bool equalAxis,
                        bool autoFitX,
                        bool autoFitY) {

        if (title.empty()) {
            title = "Figure " + std::to_string(viz.figureCounter);
        }

        if (viz.hasCurrentFigure) {
            if (viz.currentFigureOpen) {
                ImPlot::EndPlot();
            }
            ImGui::End();
        }

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

        if (ImGui::Begin(title.c_str())) {
            viz.currentFigureOpen = 
                ImPlot::BeginPlot(title.c_str(),
                              xLabel.c_str(),
                              yLabel.c_str(),
                              ImGui::GetContentRegionAvail(),
                              flags,
                              xFlags,
                              yFlags);
        } else {
            viz.currentFigureOpen = false;
        }

        viz.hasCurrentFigure = true;
        viz.figureCounter += 1;

        return viz.currentFigureOpen;
    },
    py::arg("title") = "",
    py::arg("x_label") = "",
    py::arg("y_label") = "",
    py::arg("equal_axis") = false,
    py::arg("auto_fit_x") = false,
    py::arg("auto_fit_y") = false);

    m.def("begin", [&](std::string title, bool* open) {

        return ImGui::Begin(title.c_str(), open);
    },
    py::arg("title") = "",
    py::arg("open") = true);

    m.def("end", [&]() {

        ImGui::End();
    });

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

    m.def("button", [&](std::string title) {

        return ImGui::Button(title.c_str());
    },
    py::arg("title") = "");

    m.def("begin", [&](std::string title, bool* open) {

        return ImGui::Begin(title.c_str(), open);
    },
    py::arg("title") = "",
    py::arg("open") = true);

    m.def("end", [&]() {

        ImGui::End();
    });

    m.def("text", [&](std::string str, std::vector<float> c) {

        if (c.size() == 3) {
            ImGui::TextColored(ImVec4(c[0], c[1], c[2], 1.0), "%s", str.c_str());
        } else if (c.size() == 4) {
            ImGui::TextColored(ImVec4(c[0], c[1], c[2], c[3]), "%s", str.c_str());
        } else {
            ImGui::Text("%s", str.c_str());
        }
    },
    py::arg("str"),
    py::arg("color") = std::vector<float>{});

    m.def("input", [&](std::string title, std::string& obj) {
        
        bool mod = ImGui::InputText(title.c_str(), &obj);
        return py::make_tuple(obj, mod);
    }, 
    py::arg("title"),
    py::arg("obj"));

    m.def("input", [&](std::string title, int& obj) {
        
        bool mod = ImGui::InputInt(title.c_str(), &obj);
        return py::make_tuple(obj, mod);
    }, 
    py::arg("title"),
    py::arg("obj"));

    m.def("input", [&](std::string title, float& obj) {
        
        bool mod = ImGui::InputFloat(title.c_str(), &obj);
        return py::make_tuple(obj, mod);
    }, 
    py::arg("title"),
    py::arg("obj"));

    m.def("input", [&](std::string title, double& obj) {
        
        bool mod = ImGui::InputDouble(title.c_str(), &obj);
        return py::make_tuple(obj, mod);
    }, 
    py::arg("title"),
    py::arg("obj"));

    m.def("checkbox", [&](std::string title, bool& obj) {
        
        bool mod = ImGui::Checkbox(title.c_str(), &obj);
        return py::make_tuple(obj, mod);
    }, 
    py::arg("title"),
    py::arg("obj"));

    m.def("slider", [&](std::string title, float& value, float min, float max) {
        
        bool mod = ImGui::SliderFloat(title.c_str(), &value, min, max);

        return py::make_tuple(value, mod);
    }, 
    py::arg("title"),
    py::arg("value"),
    py::arg("min") = 0.0,
    py::arg("max") = 1.0);

    m.def("drag", [&](std::string title, float& value, float speed, float min, float max) {
        
        bool mod = ImGui::DragFloat(title.c_str(), &value, speed, min, max);

        return py::make_tuple(value, mod);
    }, 
    py::arg("title"),
    py::arg("value"),
    py::arg("speed") = 0.1,
    py::arg("min") = 0.0,
    py::arg("max") = 0.0);

    m.def("range", [&](std::string title, py::tuple range, float speed, int min, int max) {

        int minVal = py::cast<int>(range[0]);
        int maxVal = py::cast<int>(range[1]);
        
        bool mod = ImGui::DragIntRange2(title.c_str(), &minVal, &maxVal, speed, min, max);

        if (ImGui::BeginPopupContextItem(title.c_str())) {

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

        return py::make_tuple(py::make_tuple(minVal, maxVal), mod);
    }, 
    py::arg("title"),
    py::arg("range"),
    py::arg("speed") = 1.0f,
    py::arg("min") = 0,
    py::arg("max") = 0);

    m.def("range", [&](std::string title, py::tuple range, float speed, float min, float max) {

        float minVal = py::cast<float>(range[0]);
        float maxVal = py::cast<float>(range[1]);
        
        bool mod = ImGui::DragFloatRange2(title.c_str(), &minVal, &maxVal, speed, min, max);

        if (ImGui::BeginPopupContextItem(title.c_str())) {

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

        return py::make_tuple(py::make_tuple(minVal, maxVal), mod);
    }, 
    py::arg("title"),
    py::arg("range"),
    py::arg("speed") = 1.0f,
    py::arg("min") = 0,
    py::arg("max") = 0);

    m.def("same_line", []() {
        ImGui::SameLine();
    });

    m.def("multiselect", [&](
                std::string label,
                py::list& values,
                py::list& selection) {

        bool modified = false;

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

                    modified = true;
                }
            }

            ImGui::EndPopup();
        }

        if (ImGui::Button(label.c_str())) {
            ImGui::OpenPopup(label.c_str());
        }

        return modified;
    },
    py::arg("label"),
    py::arg("values"),
    py::arg("selection"));

    m.def("imshow", [&](
                std::string id,
                py::array& image,
                int displayWidth,
                int displayHeight) {

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
        } else {
            throw std::runtime_error("Image format not understood");
        }

        if (channels == 1) {
            format = GL_RED;
        } else if (channels == 3) {
            format = GL_RGB;
        } else if (channels == 4) {
            format = GL_RGBA;
        } else {
            throw std::runtime_error("Channel count not understood");
        }

        if (py::str(image.dtype()).equal(py::str("uint8"))) {
            datatype = GL_UNSIGNED_BYTE;
        } else if (py::str(image.dtype()).equal(py::str("float32"))) {
            datatype = GL_FLOAT;
        } else {
            throw std::runtime_error("Pixel datatype not supported");
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

        // display texture

        if (displayWidth < 1) {
            displayWidth = imageWidth;
        }
        if (displayHeight < 1) {
            displayHeight = imageHeight;
        }

        ImPlotPoint boundsMin(0, 0);
        ImPlotPoint boundsMax(imageWidth, imageHeight);

        ImPlot::PlotImage(id.c_str(), (void*)(intptr_t)textureId, boundsMin, boundsMax);
    },
    py::arg("id"),
    py::arg("image"),
    py::arg("width") = 0,
    py::arg("height") = 0);

    m.def("dataframe", [&](
                py::object frame,
                std::string title,
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

        if (ImGui::BeginTable(title.c_str(), columnCount, flags)) {

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
    },
    py::arg("frame"),
    py::arg("title") = "",
    py::arg("selection") = py::list{});

    m.def("begin_tab_bar", [&](std::string& name) {
        return ImGui::BeginTabBar(name.c_str());
    });
    m.def("end_tab_bar", ImGui::EndTabBar);

    m.def("begin_tab_item", [&](std::string& name) {
        return ImGui::BeginTabItem(name.c_str());
    });
    m.def("end_tab_item", ImGui::EndTabItem);

    m.def("plot", [&](py::array_t<double, py::array::c_style
                        | py::array::forcecast> x,
                      py::array_t<double, py::array::c_style
                        | py::array::forcecast> y,
                      std::string fmt,
                      std::string label,
                      std::string xLabel,
                      std::string yLabel,
                      py::array_t<double, py::array::c_style
                        | py::array::forcecast> shade,
                      float shadeAlpha,
                      float lineWeight) {

        std::string title;

        if (!viz.hasCurrentFigure) {

            title = "Figure " + std::to_string(viz.figureCounter);

            if (ImGui::Begin(title.c_str())) {
                viz.currentFigureOpen = 
                    ImPlot::BeginPlot(title.c_str(),
                                  xLabel.c_str(),
                                  yLabel.c_str(),
                                  ImGui::GetContentRegionAvail());
            } else {
                viz.currentFigureOpen = false;
            }

            viz.hasCurrentFigure = true;
            viz.figureCounter += 1;
        }

        if (viz.currentFigureOpen) {

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
                //std::cout << "Cannot plot " << title << std::endl;
            } else if (groups[1] == "-") {
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

            ImPlot::PopStyleVar(2);
        }
    },
    py::arg("x"),
    py::arg("y") = py::array(),
    py::arg("fmt") = "-",
    py::arg("label") = "line",
    py::arg("x_label") = "",
    py::arg("y_label") = "",
    py::arg("shade") = py::array(),
    py::arg("shade_alpha") = 0.3f,
    py::arg("line_weight") = 1.0f);

    m.def("drag_point", [&](std::string label,
                            py::array_t<double, py::array::c_style
                                | py::array::forcecast> point,
                            bool showLabel,
                            py::array_t<double, py::array::c_style
                                | py::array::forcecast> color,
                            double radius) {

        double x = point.data()[0];
        double y = point.data()[1];

        size_t colorLength = color.shape()[0];

        ImVec4 c(0, 0, 0, 1);

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

        bool mod = ImPlot::DragPoint(label.c_str(), &x, &y, showLabel, c, radius);

        return py::make_tuple(py::make_tuple(x, y), mod);
    },
    py::arg("label"),
    py::arg("point"),
    py::arg("show_label") = false,
    py::arg("color") = py::array_t<double>(),
    py::arg("radius") = 4.0) ;

}
