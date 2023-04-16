#pragma once

#include <regex>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "implot.h"

struct ImViz {

    GLFWwindow* window = nullptr;

    ImGuiContext* imGuiCtx = nullptr;
    ImPlotContext* imPlotCtx = nullptr;

    ImFont* smallFont = nullptr;
    ImFont* largeFont = nullptr;
    double fontBaseSize = 20.0;

    ImGuiID mainDockSpaceId;

    bool currentWindowOpen = false;
    bool figurePlotOpen = false;

    bool plotPopupOpen = false;
    size_t plotPopupId = 0;
    ImPlotPoint plotPopupPoint{0.0, 0.0};

    bool initialized = false;

    bool mod = false;
    std::vector<bool> mod_any = {false};

    std::string iniFilePath = "";

    // initially update for two whole seconds (assuming vsync)
    int powerSaveFrameCounter = 120;

    ImViz() = default;

    void init();
    void prepareUpdate();
    void reloadFonts();
    void setupImLibs();
    void doUpdate(bool useVsync);
    void recover();
    void trigger();
    void setMod(bool m);
};
