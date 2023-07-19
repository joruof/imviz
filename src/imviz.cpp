#include "imviz.hpp"

#include <imgui.h>
#include <iostream>

#include "input.hpp"
#include "source_sans_pro.hpp"

#include "imgui_internal.h"
#include "implot_internal.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

void error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW error %d - %s\n", error, description);
}

// Doing this in the constructor directly breaks on Windows
void ImViz::init() {
    if (this->initialized) {
        return;
    }

    if (!glfwInit()) {
        std::cout << "Could not initialize GLFW!" << std::endl;
        std::exit(-1);
    }

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);

    // Required on MacOS
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    window = glfwCreateWindow(
            800,
            600,
            "imviz",
            nullptr,
            nullptr);

    if (!window) {
        printf("Window creation failed!\n");
        exit(1);
    }

    glfwMakeContextCurrent(window);

    glewExperimental = true;

    if (GLEW_OK != glewInit()) {
        std::cout << "GL Extension Wrangler initialization failed!"
                  << std::endl;
        std::exit(-1);
    }

    setupImLibs();

    prepareUpdate();

    this->initialized = true;
}

void ImViz::reloadFonts () {

    ImGuiIO& io = ImGui::GetIO();

    if (smallFont == nullptr
        || largeFont == nullptr
        || smallFont->FontSize != fontBaseSize) {

        io.Fonts->Clear();

        smallFont = io.Fonts->AddFontFromMemoryCompressedTTF(
                getSourceSansProData(),
                getSourceSansProSize(),
                fontBaseSize);

        largeFont = io.Fonts->AddFontFromMemoryCompressedTTF(
                getSourceSansProData(),
                getSourceSansProSize(),
                100.0);

        ImGui_ImplOpenGL3_CreateFontsTexture();
    }
}

void ImViz::setupImLibs() {

    if (imGuiCtx != nullptr) {
        ImGui_ImplGlfw_Shutdown();
    }
    if (imGuiCtx != nullptr) { 
        ImGui::DestroyContext(imGuiCtx);
    }
    if (imPlotCtx != nullptr) { 
        ImPlot::DestroyContext(imPlotCtx);
    }

    // input setup
    
    input::registerCallbacks(window);

    // basic imgui setup

    IMGUI_CHECKVERSION();
    imGuiCtx = ImGui::CreateContext();
    imPlotCtx = ImPlot::CreateContext();

    ImGui::SetCurrentContext(imGuiCtx);
    ImPlot::SetCurrentContext(imPlotCtx);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = NULL;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

    reloadFonts();
}

void ImViz::prepareUpdate() {

    input::update();

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantCaptureMouse) {
        input::clearMouseInput();
    }
    if (io.WantCaptureKeyboard) {
        input::clearKeyboardInput();
    }

    reloadFonts();

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

    mainDockSpaceId = ImGui::DockSpace(1, ImVec2(0.0f, 0.0f), dockspace_flags);

    ImGui::End();
}

void ImViz::doUpdate (bool useVsync) {

    currentWindowOpen = false;

    glfwShowWindow(window);

    // Try soft error recovery. At first we do not destroy the imgui context.
    // If recover fails, the second recovery stage will recreate the context.
    // (context recreation implemented in wait() method)
    recover();

    // ensure that the default framebuffer is always bound

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    ImGui::Render();

    // background color taken from the one-and-only tomorrow-night theme

    glClearColor(0.11372549019607843,
                 0.12156862745098039,
                 0.12941176470588237,
                 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // this is here to ensure that images of any size can
    // be loaded correctly from their raw image data
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int display_w, display_h;
    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &display_w, &display_h);

    glViewport(0, 0, display_w, display_h);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwMakeContextCurrent(window);

    glfwSwapInterval(useVsync);
    glfwSwapBuffers(window);

    ImGuiIO& io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void ImViz::recover()
{
    // ImGui::ErrorCheckEndFrameRecover extended for implot and mod any

    mod_any = {false};

    ImPlotContext& gp = *GImPlot;

    while (gp.CurrentPlot != NULL) {

        while (gp.CurrentItem != NULL) {
            ImPlot::EndItem();
        }
        while (gp.CurrentSubplot != NULL) {
            ImPlot::EndSubplots();
        }
        while (gp.CurrentPlot != NULL) {
            ImPlot::EndPlot();
        }
    }

    ImGui::ErrorCheckEndFrameRecover(NULL);
}

void ImViz::trigger () {

    glfwPostEmptyEvent();
}

void ImViz::setMod(bool m) {

    mod = m;
    mod_any.back() = mod_any.back() | m;
}
