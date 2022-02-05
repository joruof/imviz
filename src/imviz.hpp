#pragma once

#include <regex>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "implot.h"

struct ImViz {

    GLFWwindow* window = nullptr;

    ImGuiContext* imGuiCtx = nullptr;
    ImPlotContext* imPlotCtx = nullptr;

    bool currentWindowOpen = false;
    bool figurePlotOpen = false;

    bool mod = false;
    bool mod_any = false;

    std::string iniFilePath = "";

    // initially update for two whole seconds (assuming vsync)
    int powerSaveFrameCounter = 120;

    std::regex re{"(-)?(o|s|d|\\*|\\+)?"};

    ImViz();

    void prepareUpdate();
    void setupImLibs();
    void doUpdate(bool useVsync);
    void recover();
    void trigger();
    void setMod(bool m);
};
