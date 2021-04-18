#include <imgui.h>
#include <regex>
#include <functional>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/pytypes.h>
#include <pybind11/pybind11.h>

#include <experimental/filesystem>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "implot.h"

#include "SourceSansPro.h"

struct PyImPlot {

    GLFWwindow* window = nullptr;

    bool hasCurrentFigure = false;
    bool currentFigureOpen = false;

    size_t figureCounter = 1;

    std::regex re{"(-)?(o|s|d|\\*|\\+)?(r|g|b|y|m|w)?"};

    PyImPlot () {

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
                "pyimplot",
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
        io.IniFilename = "./pyimplot.ini";

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

        static bool lightTheme = false;

        if (ImGui::BeginMainMenuBar()) {

            if (ImGui::BeginMenu("Show")) {

                ImGui::MenuItem("Light Theme", NULL, &lightTheme);
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (lightTheme) {
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

        ImPlot::ShowDemoWindow();
    }

    void doUpdate () {

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
        
        // background color take from the one-and-only tomorrow-night theme

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

        glfwSwapBuffers(window);

        figureCounter = 0;
    }

    void trigger () {

        glfwPostEmptyEvent();
    }
};

PyImPlot plt;

PYBIND11_MODULE(pyimplot, m) {

    m.def("wait", [&]() {
        plt.doUpdate();
        glfwPollEvents();
        plt.prepareUpdate();
        return !glfwWindowShouldClose(plt.window);
    });

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
        plt.trigger();
    });

    m.def("figure", [&](std::string title) {

        if (title.empty()) {
            title = "Figure " + std::to_string(plt.figureCounter);
        }

        if (plt.hasCurrentFigure) {
            if (plt.currentFigureOpen) {
                ImPlot::EndPlot();
            }
            ImGui::End();
        }

        if (ImGui::Begin(title.c_str())) {
            ImPlot::BeginPlot(title.c_str(),
                              NULL,
                              NULL,
                              ImGui::GetContentRegionAvail());
            plt.currentFigureOpen = true;
        } else {
            plt.currentFigureOpen = false;
        }

        plt.hasCurrentFigure = true;
        plt.figureCounter += 1;

        return plt.currentFigureOpen;
    },
    pybind11::arg("title") = "");

    m.def("plot", [&](pybind11::array_t<float, pybind11::array::c_style
                        | pybind11::array::forcecast> x,
                      pybind11::array_t<float, pybind11::array::c_style
                        | pybind11::array::forcecast> y,
                      std::string fmt,
                      std::string label) {

        std::string title;

        if (!plt.hasCurrentFigure) {

            title = "Figure " + std::to_string(plt.figureCounter);

            if (ImGui::Begin(title.c_str())) {
                ImPlot::BeginPlot(title.c_str(),
                                  NULL,
                                  NULL,
                                  ImGui::GetContentRegionAvail());
                plt.currentFigureOpen = true;
            } else {
                plt.currentFigureOpen = false;
            }

            plt.hasCurrentFigure = true;
            plt.figureCounter += 1;
        }

        if (plt.currentFigureOpen) {

            std::smatch match;
            std::regex_search(fmt, match, plt.re);

            std::vector<std::string> groups;
            for (auto m : match) {
                if (m.matched) {
                    groups.push_back(m.str());
                } else {
                    groups.push_back("");
                }
            }

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

            std::vector<float> indices;
            const float* xDataPtr = nullptr;
            const float* yDataPtr = nullptr;
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

            ImPlot::PopStyleVar(1);
        }
    },
    pybind11::arg("x"),
    pybind11::arg("y") = pybind11::array(),
    pybind11::arg("fmt") = "-",
    pybind11::arg("label") = "line");
}
