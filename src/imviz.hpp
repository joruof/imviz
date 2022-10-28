#pragma once

#include <regex>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "implot.h"

struct ImViz {

    GLFWwindow* window = nullptr;

    ImGuiContext* imGuiCtx = nullptr;
    ImPlotContext* imPlotCtx = nullptr;

    ImFont* smallFont;
    ImFont* largeFont;

    ImGuiID mainDockSpaceId;

    bool currentWindowOpen = false;
    bool figurePlotOpen = false;
    bool initialized = false;

    bool mod = false;
    std::vector<bool> mod_any = {false};

    float rotation = 0.0f;
    int rotationStartIndex = 0;

    std::string iniFilePath = "";

    // initially update for two whole seconds (assuming vsync)
    int powerSaveFrameCounter = 120;

    ImViz() = default;

    void init();
    void prepareUpdate();
    void setupImLibs();
    void doUpdate(bool useVsync);
    void recover();
    void trigger();
    void setMod(bool m);
};
