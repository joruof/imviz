#include "imviz.hpp"
#include <stdexcept>

#define EGL_EGLEXT_PROTOTYPES
#include <GL/glew.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <imgui.h>
#include <iostream>

#include "input.hpp"
#include "source_sans_pro.hpp"
#include "fa_solid_900.hpp"

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

    if (glfwInit()) {

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
            throw std::runtime_error("GLFW window creation failed!\n");
            exit(1);
        }

        glfwMakeContextCurrent(window);
    } else {
        std::cerr << "Cannot initialize GLFW, using headless mode" << std::endl;

        /**
         * First we need to open an EGL display.
         *
         * For some reason we cannot simple call eglGetDisplay(...) in docker.
         * Instead we need to do the following:
         */

        static const int MAX_DEVICES = 32;
        EGLDeviceEXT eglDevs[MAX_DEVICES];
        EGLint numDevices;

        PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT =
          (PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress("eglQueryDevicesEXT");

        eglQueryDevicesEXT(MAX_DEVICES, eglDevs, &numDevices);

        if (0 == numDevices) {
            throw std::runtime_error("Found 0 EGL capable devices!");
        }

        PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
          (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress(
              "eglGetPlatformDisplayEXT");

        const char* selectedDevice = std::getenv("CUDA_VISIBLE_DEVICES");
        size_t gpuId = 0;

        if (selectedDevice != nullptr) {
            gpuId = (size_t)std::min(MAX_DEVICES, std::stoi(selectedDevice));
        }

        EGLDisplay eglDpy =
          eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, eglDevs[gpuId], 0);

        if (0 == eglDpy) {
            throw std::runtime_error("EGL display creation has failed!");
        }

        // ok we got our virtual display, initialize

        EGLint major = 0;
        EGLint minor = 0;

        if (!eglInitialize(eglDpy, &major, &minor)) {
            throw std::runtime_error("EGL initialization has failed!");
        }

        // create virtual surface for rendering

        EGLint numConfigs;
        EGLConfig eglCfg;

        const EGLint configAttribs[] = {
              EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
              EGL_BLUE_SIZE, 8,
              EGL_GREEN_SIZE, 8,
              EGL_RED_SIZE, 8,
              EGL_DEPTH_SIZE, 8,
              EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
              EGL_NONE
        };

        const EGLint pbufferAttribs[] = {
            EGL_WIDTH, 1920,
            EGL_HEIGHT, 1080,
            EGL_NONE,
        };

        eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs);
        EGLSurface eglSurf = eglCreatePbufferSurface(
                eglDpy, eglCfg, pbufferAttribs);

        if (0 == eglSurf) {
            throw std::runtime_error("EGL surface creation has failed!");
        }

        eglBindAPI(EGL_OPENGL_API);

        // create opengl context

        EGLContext eglCtx = eglCreateContext(
                eglDpy, eglCfg, EGL_NO_CONTEXT, NULL);
        eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);

        if (0 == eglCtx) {
            throw std::runtime_error("EGL context creation has failed!");
        }
    }

    glewExperimental = true;

    GLenum initResult = glewInit();

    // Complaining about having no GLX display, is ok in EGL mode,
    // as there is (by definition) no X-Server involved.
    if (GLEW_ERROR_NO_GLX_DISPLAY != initResult && GLEW_OK != initResult) {
        throw std::runtime_error("GLEW initialization with EGL has failed!");
    }

    setupImLibs();

    prepareUpdate();

    this->initialized = true;
}

void ImViz::setWindowSize(ImVec2 size) {

    if (nullptr != window) {
        glfwSetWindowSize(window, (int)size.x, (int)size.y);
    } else {
        eglWindowWidth = (int)size.x;
        eglWindowHeight = (int)size.y;

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = size;
    }
}

ImVec2 ImViz::getWindowSize() {

    int w = 0;
    int h = 0;

    if (nullptr != window) {
        glfwGetWindowSize(window, &w, &h);
    } else {
        w = eglWindowWidth;
        h = eglWindowHeight;
    }

    return ImVec2(w, h);
}

void ImViz::reloadFonts () {

    ImGuiIO& io = ImGui::GetIO();

    if (smallFont == nullptr
        || iconFont == nullptr
        || smallFont->FontSize != fontBaseSize) {

        io.Fonts->Clear();

        ImGui_ImplOpenGL3_DestroyFontsTexture();

        smallFont = io.Fonts->AddFontFromMemoryCompressedTTF(
                getSourceSansProData(),
                getSourceSansProSize(),
                fontBaseSize);

        const float iconFontSize = fontBaseSize * 2.0f / 3.0f;

        ImFontConfig iconsConfig;
        iconsConfig.MergeMode = true;
        iconsConfig.PixelSnapH = true;
        iconsConfig.GlyphMinAdvanceX = iconFontSize*1.3;

        static const ImWchar iconsRanges[] = {0xe005, 0xf8ff, 0};

        iconFont = io.Fonts->AddFontFromMemoryCompressedTTF(
                getFontAwesomeSolid900Data(),
                getFontAwesomeSolid900Size(),
                iconFontSize,
                &iconsConfig,
                iconsRanges);

        ImGui_ImplOpenGL3_CreateFontsTexture();
    }
}

void ImViz::setupImLibs() {

    if (imGuiCtx != nullptr && window != nullptr) {
        ImGui_ImplGlfw_Shutdown();
    }
    if (imGuiCtx != nullptr) { 
        ImGui::DestroyContext(imGuiCtx);
    }
    if (imPlotCtx != nullptr) { 
        ImPlot::DestroyContext(imPlotCtx);
    }

    // input setup
    
    if (window != nullptr) {
        input::registerCallbacks(window);
    }

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

    if (window != nullptr) {
        ImGui_ImplGlfw_InitForOpenGL(window, true);
    } else {
        io.DisplaySize = ImVec2(eglWindowWidth, eglWindowHeight);
    }
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
    if (window != nullptr) {
        ImGui_ImplGlfw_NewFrame();
    }
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

    if (nullptr != window) {
        glfwShowWindow(window);
    }

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
    if (nullptr != window) {
        glfwMakeContextCurrent(window);
        glfwGetFramebufferSize(window, &display_w, &display_h);
    } else {
        display_w = eglWindowWidth;
        display_h = eglWindowHeight;
    }

    glViewport(0, 0, display_w, display_h);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (nullptr != window) {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(useVsync);
        glfwSwapBuffers(window);
    }

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
