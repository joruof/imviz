#include "file_dialog.hpp"

#include <vector>
#include <algorithm>
#include <filesystem>
#include <iostream>

#include "imgui.h"
#include "imgui_internal.h"

namespace fs = std::filesystem;

/**
 * Custom ImGui Extension for handling path selection.
 */
namespace ImGui {

    void PathSelector (std::string& selectedPath) { 

        ImGui::BeginChild("Dir Listing",
                ImVec2(600, 400),
                false,
                ImGuiWindowFlags_HorizontalScrollbar
                | ImGuiWindowFlags_NoResize);

        if (ImGui::Selectable("./..", false)) {
            if (!fs::is_directory(selectedPath) 
                    || fs::path(selectedPath).filename().empty()) {
                selectedPath = fs::path(selectedPath)
                    .parent_path()
                    .parent_path()
                    .string();
            } else {
                selectedPath = fs::path(selectedPath)
                    .parent_path().string();
            }
        }

        // Obtain a list of all entries in the current directory

        fs::path listPath = selectedPath;
        if (!fs::is_directory(listPath)) {
            listPath = listPath.parent_path();
        }

        std::vector<fs::directory_entry> entries;

        if (fs::exists(listPath)) {
            for (fs::directory_entry e : fs::directory_iterator(listPath)) {
                if (e.path().filename().string().at(0) != '.') {
                    entries.push_back(e);
                }
            }
        }

        // Because the entries may be sorted arbitrarily
        // we sort them alphabetically.
        // Also directories are sorted in before files.

        std::sort(
                entries.begin(),
                entries.end(),
                [](auto& a, auto& b) -> bool {
                    if (fs::is_directory(a) && !fs::is_directory(b)) {
                        return true;
                    } else if (!fs::is_directory(a) && fs::is_directory(b)) {
                        return false;
                    } else {
                        return a.path().filename() < b.path().filename();
                    }
                }
            );

        for (fs::directory_entry e : entries) {

            std::string displayName = e.path().stem().string();
            displayName += e.path().extension().string();

            if (fs::is_directory(e)) {
                displayName += "/";
            }

            if (ImGui::Selectable(
                        displayName.c_str(),
                        selectedPath == e.path().string())) {

                selectedPath = e.path().string();
            }
        }

        ImGui::EndChild();

        char filenameInputBuf[1024];

        strncpy(filenameInputBuf, 
                selectedPath.c_str(), 
                sizeof(filenameInputBuf) - 1);

        float width = 600 * 0.8f;
        ImGui::PushItemWidth(width);
        ImGui::InputText("selected",
                filenameInputBuf, IM_ARRAYSIZE(filenameInputBuf));
        ImGui::PopItemWidth();

        selectedPath = std::string(filenameInputBuf);
    }

    bool IsItemActivePreviousFrame()
    {
        ImGuiContext& g = *GImGui;
        if (g.ActiveIdPreviousFrame)
            return g.ActiveIdPreviousFrame == GImGui->LastItemData.ID;
        return false;
    }

    bool FileDialogPopup (
            const char* name,
            const char* confirmLabel,
            std::string& selectedPath) {

        bool result = false;

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        static std::string currentPath = "";
        static bool fileDialogOpen = false;

        if (ImGui::BeginPopupModal(
                    name, 
                    NULL, 
                    ImGuiWindowFlags_AlwaysAutoResize)) {

            if (!fileDialogOpen) {
                currentPath = selectedPath;
                currentPath = fs::absolute(currentPath);
                while (!fs::exists(currentPath)) {
                    currentPath = fs::path(currentPath).parent_path();
                }
            }

            std::string basedir;
            if (!fs::is_directory(currentPath)
                    || fs::path(currentPath).filename().empty()) {
                basedir = fs::path(currentPath)
                    .parent_path()
                    .filename();
            } else {
                basedir = fs::path(currentPath).filename();
            }

            ImGui::Text("%s", ("Directory: " + basedir).c_str());
            ImGui::Separator();

            ImGui::PathSelector(currentPath);

            bool pressed_enter = IsItemActivePreviousFrame()
                                 && !IsItemActive()
                                 && IsKeyPressed(ImGuiKey_Enter);
            pressed_enter = pressed_enter
                            || IsKeyPressed(ImGuiKey_Enter);

            bool pressed_escape = IsItemActivePreviousFrame()
                                  && !IsItemActive()
                                  && IsKeyPressed(ImGuiKey_Escape);
            pressed_escape = pressed_escape
                             || IsKeyPressed(ImGuiKey_Escape);

            fileDialogOpen = true;

            if (pressed_enter || ImGui::Button(confirmLabel, ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                fileDialogOpen = false;
                selectedPath = currentPath;
                result = true;
            }

            ImGui::SameLine();

            if (pressed_escape || ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                fileDialogOpen = false;
                result = false;
            }

            ImGui::EndPopup();
        }

        return result;
    }
}
