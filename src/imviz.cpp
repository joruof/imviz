#include "imviz.hpp"

#include <iostream>

#include "input.hpp"
#include "source_sans_pro.hpp"

#include "imgui_internal.h"
#include "implot_internal.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

ImViz::ImViz () {

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
        std::cout << "GL Extension Wrangler initialization failed!"
                  << std::endl;
        std::exit(-1);
    }

    setupImLibs();

    prepareUpdate();
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

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = NULL;

    ImPlot::GetStyle().AntiAliasedLines = true;

    // loading font

    io.Fonts->AddFontFromMemoryCompressedTTF(
            getSourceSansProData(),
            getSourceSansProSize(),
            20.0);

    ImGui_ImplOpenGL3_CreateFontsTexture();
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

    ImGui::DockSpace(1, ImVec2(0.0f, 0.0f), dockspace_flags);

    ImGui::End();
}

void ImViz::doUpdate (bool useVsync) {

    currentWindowOpen = false;

    glfwShowWindow(window);

    // Try soft error recovery. At first we do not destroy the imgui context.
    // If recover fails, the second recovery stage will recreate the context.
    // (context receration implemented in wait() method)
    recover();

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

void ImViz::recover()
{
    //basically ImGui::ErrorCheckEndFrameRecover
    //extended for implot

    ImGuiContext& g = *GImGui;
    ImPlotContext& gp = *GImPlot;

    while (g.CurrentWindowStack.Size > 0)
    {
        #ifdef IMGUI_HAS_TABLE
        while (g.CurrentTable
                && (g.CurrentTable->OuterWindow == g.CurrentWindow
                    || g.CurrentTable->InnerWindow == g.CurrentWindow)) {
            ImGui::EndTable();
        }
        #endif

        while (gp.CurrentItem != NULL) {
            ImPlot::EndItem();
        }
        while (gp.CurrentSubplot != NULL) {
            ImPlot::EndSubplots();
        }
        while (gp.CurrentPlot != NULL) {
            ImPlot::EndPlot();
        }

        ImGuiWindow* window = g.CurrentWindow;
        IM_ASSERT(window != NULL);
        ImGuiStackSizes* stack_sizes = &g.CurrentWindowStack.back().StackSizesOnBegin;

        while (g.CurrentTabBar != NULL) { //-V1044
            ImGui::EndTabBar();
        }
        while (window->DC.TreeDepth > 0) {
            ImGui::TreePop();
        }
        while (g.GroupStack.Size > stack_sizes->SizeOfGroupStack) {
            ImGui::EndGroup();
        }
        while (window->IDStack.Size > 1) {
            ImGui::PopID();
        }
        while (g.ColorStack.Size > stack_sizes->SizeOfColorStack) {
            ImGui::PopStyleColor();
        }
        while (g.StyleVarStack.Size > stack_sizes->SizeOfStyleVarStack) {
            ImGui::PopStyleVar();
        }
        while (g.FocusScopeStack.Size > stack_sizes->SizeOfFocusScopeStack) {
            ImGui::PopFocusScope();
        }
        if (g.CurrentWindowStack.Size == 1) {
            IM_ASSERT(g.CurrentWindow->IsFallbackWindow);
            break;
        }

        IM_ASSERT(window == g.CurrentWindow);

        if (window->Flags & ImGuiWindowFlags_ChildWindow) {
            ImGui::EndChild();
        }
        else {
            ImGui::End();
        }
    }
}

void ImViz::trigger () {

    glfwPostEmptyEvent();
}

void ImViz::setMod(bool m) {

    mod = m;
    mod_any |= m;
}
