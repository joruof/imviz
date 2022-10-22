#pragma once

#include <string>

namespace ImGui {

    void PathSelector (std::string& selectedPath);
    bool FileDialogPopup (const char* name, const char* confirmLabel, std::string& selectedPath);
} 
