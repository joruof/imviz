#include <functional>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/pytypes.h>
#include <pybind11/pybind11.h>

#include <experimental/filesystem>

#include "ocornut_imgui/imgui_impl_glfw.h"
#include "ocornut_imgui/imgui_impl_opengl3.h"
#include "ocornut_imgui/implot.h"

namespace fs = std::experimental::filesystem;

std::function<void()> updateFunc = nullptr; 
GLFWwindow* window = nullptr;

PYBIND11_MODULE(pyimplot, m) {

    if (!glfwInit()) {
        std::cout << "Could not initialize GLFW!" << std::endl;
        std::exit(-1);
    }

    glfwWindowHint(GLFW_SAMPLES, 1);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);

    window = glfwCreateWindow(
            800,
            600,
            "SpatzSim",
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

    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGui::StyleColorsDark();

    // loading font

    /*
    fs::path fsResPath = "./"
    fsResPath = fsResPath / "noto_sans_regular.ttf";

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(fsResPath.c_str(), 15.0);
    */

    glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
    glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
    glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
    glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);

    auto prepareUpdate = [&]() {

        glfwPollEvents(); 

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = "./pyimplot.ini";

        if (io.WantCaptureMouse) {
            // do something ... 
        }
        if (io.WantCaptureKeyboard) {
            // do something ... 
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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
        ImGui::SetNextWindowPos(viewport->GetWorkPos());
        ImGui::SetNextWindowSize(viewport->GetWorkSize());
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("Main DockSpace", NULL, window_flags);

        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();

        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None
                | ImGuiDockNodeFlags_NoDockingInCentralNode
                | ImGuiDockNodeFlags_PassthruCentralNode;

        ImGui::DockSpace(1, ImVec2(0.0f, 0.0f), dockspace_flags);

        ImGui::End();

        ImPlot::ShowDemoWindow();
    };

    prepareUpdate();

    glfwPostEmptyEvent();

    auto doUpdate = [&]() {

        glfwShowWindow(window);

        ImGui::Render();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int display_w, display_h;
        glfwMakeContextCurrent(window);
        glfwGetFramebufferSize(window, &display_w, &display_h);

        glViewport(0, 0, display_w, display_h);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwMakeContextCurrent(window);

        glfwSwapBuffers(window);

        prepareUpdate();

        return !glfwWindowShouldClose(window);
    };

    m.def("wait", [&]() {
        glfwWaitEvents();
        return doUpdate();
    });

    m.def("show", [&]() {
        while(doUpdate()) {
            glfwWaitEvents();
        }
    });
}
