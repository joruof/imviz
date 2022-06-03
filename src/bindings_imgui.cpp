#include "bindings_implot.hpp"

#include "binding_helpers.hpp"
#include "imviz.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"

static py::object dragDropRef = py::none();
static int dragDropClearCounter = 0;

void loadImguiPythonBindings(pybind11::module& m, ImViz& viz) {

    /**
     * Flags and defines
     */

    py::enum_<ImGuiCond_>(m, "Cond")
        .value("NONE", ImGuiCond_None)
        .value("ALWAYS", ImGuiCond_Always)
        .value("ONCE", ImGuiCond_Once)
        .value("FIRST_USE_EVER", ImGuiCond_FirstUseEver)
        .value("APPEARING", ImGuiCond_Appearing);

    py::enum_<ImGuiDragDropFlags_>(m, "DragDropFlags", py::arithmetic())
        .value("NONE", ImGuiDragDropFlags_None)
        .value("SOURCE_EXTERN", ImGuiDragDropFlags_SourceExtern)
        .value("SOURCE_ALLOW_NULL_ID", ImGuiDragDropFlags_SourceAllowNullID)
        .value("SOURCE_NO_DISABLE_HOVER", ImGuiDragDropFlags_SourceNoDisableHover)
        .value("SOURCE_NO_PREVIEW_TOOLTIP", ImGuiDragDropFlags_SourceNoPreviewTooltip)
        .value("SOURCE_AUTO_EXPIRE_PAYLOAD", ImGuiDragDropFlags_SourceAutoExpirePayload)
        .value("SOURCE_NO_HOLD_TO_OPEN_OTHERS", ImGuiDragDropFlags_SourceNoHoldToOpenOthers)
        .value("ACCEPT_PEEK_ONLY", ImGuiDragDropFlags_AcceptPeekOnly)
        .value("ACCEPT_BEFORE_DELIVERY", ImGuiDragDropFlags_AcceptBeforeDelivery)
        .value("ACCEPT_NO_PREVIEW_TOOLTIP", ImGuiDragDropFlags_AcceptNoPreviewTooltip)
        .value("ACCEPT_NO_DRAW_DEFAULT_RECT", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

    py::enum_<ImDrawFlags_>(m, "DrawFlags")
        .value("NONE", ImDrawFlags_None)
        .value("CLOSED", ImDrawFlags_Closed)
        .value("ROUND_CORNERS_TOP_LEFT", ImDrawFlags_RoundCornersTopLeft)
        .value("ROUND_CORNERS_TOP_RIGHT", ImDrawFlags_RoundCornersTopRight)
        .value("ROUND_CORNERS_BOTTOM_LEFT", ImDrawFlags_RoundCornersBottomLeft)
        .value("ROUND_CORNERS_BOTTOM_RIGHT", ImDrawFlags_RoundCornersBottomRight)
        .value("ROUND_CORNERS_NONE", ImDrawFlags_RoundCornersNone)
        .value("ROUND_CORNERS_TOP", ImDrawFlags_RoundCornersTop)
        .value("ROUND_CORNERS_BOTTOM", ImDrawFlags_RoundCornersBottom)
        .value("ROUND_CORNERS_LEFT", ImDrawFlags_RoundCornersLeft)
        .value("ROUND_CORNERS_RIGHT", ImDrawFlags_RoundCornersRight)
        .value("ROUND_CORNERS_ALL", ImDrawFlags_RoundCornersAll)
        .value("ROUND_CORNERS_DEFAULT", ImDrawFlags_RoundCornersDefault_)
        .value("ROUND_CORNERS_MASK", ImDrawFlags_RoundCornersMask_);

    /*
     * Imgui widgets
     */

    m.def("show_imgui_demo", ImGui::ShowDemoWindow);
    m.def("show_implot_demo", ImPlot::ShowDemoWindow);

    m.def("begin_window", [&](std::string label,
                       bool opened,
                       array_like<float> position, 
                       array_like<float> size, 
                       bool title_bar,
                       bool resize,
                       bool move,
                       bool scrollbar,
                       bool scrollWithMouse,
                       bool collapse,
                       bool autoResize) {

        if (position.shape()[0] > 0) { 
            assert_shape(position, {{2}});
            const float* data = position.data();
            ImGui::SetNextWindowPos({data[0], data[1]});
        }
        if (size.shape()[0] > 0) {
            assert_shape(size, {{2}});
            const float* data = size.data();
            ImGui::SetNextWindowSize({data[0], data[1]});
        }

        ImGuiWindowFlags flags = ImGuiWindowFlags_None;

        flags |= ImGuiWindowFlags_NoTitleBar * !title_bar;
        flags |= ImGuiWindowFlags_NoResize * !resize;
        flags |= ImGuiWindowFlags_NoMove * !move;
        flags |= ImGuiWindowFlags_NoScrollbar * !scrollbar;
        flags |= ImGuiWindowFlags_NoScrollWithMouse * !scrollWithMouse;
        flags |= ImGuiWindowFlags_NoCollapse * !collapse;
        flags |= ImGuiWindowFlags_AlwaysAutoResize * autoResize;

        viz.currentWindowOpen = opened;

        bool show = ImGui::Begin(label.c_str(), &viz.currentWindowOpen, flags);

        return show;
    },
    py::arg("label"),
    py::arg("opened") = true,
    py::arg("position") = py::array(),
    py::arg("size") = py::array(),
    py::arg("title_bar") = true,
    py::arg("resize") = true,
    py::arg("move") = true,
    py::arg("scrollbar") = true,
    py::arg("scroll_with_mouse") = true,
    py::arg("collapse") = true,
    py::arg("auto_resize") = false);

    m.def("end_window", ImGui::End);

    m.def("begin_popup_context_item", [&](std::string label) {

        return ImGui::BeginPopupContextItem(
                label.empty() ? 0 : label.c_str());
    },
    py::arg("label") = "");

    m.def("begin_popup", [&](std::string label) {

        return ImGui::BeginPopup(label.c_str());
    });

    m.def("begin_popup_modal", [&](std::string label) {

        return ImGui::BeginPopupModal(
                label.c_str(),
                NULL,
                ImGuiWindowFlags_AlwaysAutoResize);
    });

    m.def("open_popup", [&](std::string label) {

        return ImGui::OpenPopup(label.c_str());
    });

    m.def("close_current_popup", ImGui::CloseCurrentPopup);

    m.def("end_popup", ImGui::EndPopup);

    m.def("begin_main_menu_bar", ImGui::BeginMainMenuBar);
    m.def("end_main_menu_bar", ImGui::EndMainMenuBar);

    m.def("begin_menu_bar", ImGui::BeginMenuBar);
    m.def("end_menu_bar", ImGui::EndMenuBar);

    m.def("begin_menu", [&](std::string label, bool enabled) {

        return ImGui::BeginMenu(label.c_str(), enabled);
    },
    py::arg("label"),
    py::arg("enabled") = true);

    m.def("end_menu", ImGui::EndMenu);

    m.def("menu_item", [&](std::string label, std::string shortcut, bool selected, bool enabled) {
        
        return ImGui::MenuItem(label.c_str(), shortcut.c_str(), selected, enabled);
    },
    py::arg("label"),
    py::arg("shortcut") = "",
    py::arg("selected") = false,
    py::arg("enabled") = true);

    m.def("begin_tab_bar", [&](std::string& name) {
        return ImGui::BeginTabBar(name.c_str());
    });
    m.def("end_tab_bar", ImGui::EndTabBar);

    m.def("begin_tab_item", [&](std::string& name) {
        return ImGui::BeginTabItem(name.c_str());
    });
    m.def("end_tab_item", ImGui::EndTabItem);

    m.def("tree_node", [&](std::string label, bool selected) {

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
        if (selected) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        return ImGui::TreeNodeEx(label.c_str(), flags);
    },
    py::arg("label") = "",
    py::arg("selected") = false);

    m.def("tree_pop", ImGui::TreePop);

    m.def("button", [&](std::string label) {

        return ImGui::Button(label.c_str());
    },
    py::arg("label"));

    m.def("combo", [&](std::string label, py::list items, int selectionIndex) {

        size_t len = items.size();

        std::vector<std::string> objStr(len);
        std::vector<const char*> objPtr(len);

        int i = 0;
        for (const py::handle& o : items) {
            objStr[i] = py::str(o);
            objPtr[i] = objStr[i].c_str();
            i += 1;
        }

        bool mod = ImGui::Combo(label.c_str(), &selectionIndex, objPtr.data(), len);
        viz.setMod(mod);

        return selectionIndex;
    },
    py::arg("label"),
    py::arg("items"),
    py::arg("selection_index") = 0);

    m.def("text", [&](py::handle obj, array_like<double> color) {

        ImVec4 c = interpretColor(color);

        std::string str = py::str(obj);

        if (c.w >= 0) {
            ImGui::TextColored(c, "%s", str.c_str());
        } else {
            ImGui::Text("%s", str.c_str());
        }
    },
    py::arg("str"),
    py::arg("color") = py::array_t<double>());

    m.def("input", [&](std::string label, std::string& obj) {
        
        bool mod = ImGui::InputText(label.c_str(), &obj);
        viz.setMod(mod);

        return obj;
    }, 
    py::arg("label"),
    py::arg("obj"));

    m.def("input", [&](std::string label, int& obj) {
        
        bool mod = ImGui::InputInt(label.c_str(), &obj);
        viz.setMod(mod);

        return obj;
    }, 
    py::arg("label"),
    py::arg("obj"));

    m.def("input", [&](std::string label, double& obj) {
        
        bool mod = ImGui::InputDouble(label.c_str(), &obj);
        viz.setMod(mod);

        return obj;
    }, 
    py::arg("label"),
    py::arg("obj"));

    m.def("checkbox", [&](std::string label, bool& obj) {
        
        bool mod = ImGui::Checkbox(label.c_str(), &obj);
        viz.setMod(mod);

        return obj;
    }, 
    py::arg("label"),
    py::arg("obj"));

    m.def("slider", [&](std::string title, double& value, double min, double max) {
        
        bool mod = ImGui::SliderScalar(
                title.c_str(), ImGuiDataType_Double, &value, &min, &max);
        viz.setMod(mod);

        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
    py::arg("min") = 0.0,
    py::arg("max") = 1.0);

    m.def("drag", [&](std::string title, int& value, float speed, int min, int max) {
        
        bool mod = ImGui::DragInt(title.c_str(), &value, speed, min, max);
        viz.setMod(mod);

        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
    py::arg("speed") = 1.0,
    py::arg("min") = 0,
    py::arg("max") = 0);

    m.def("drag", [&](std::string title, double& value, float speed, double min, double max) {
        
        bool mod = ImGui::DragScalar(title.c_str(),
                ImGuiDataType_Double, &value, speed, &min, &max);
        viz.setMod(mod);

        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
    py::arg("speed") = 0.1,
    py::arg("min") = 0.0,
    py::arg("max") = 0.0);

    m.def("range", [&](std::string label, py::tuple range, float speed, float min, float max) {

        float minVal = py::cast<float>(range[0]);
        float maxVal = py::cast<float>(range[1]);
        
        bool mod = ImGui::DragFloatRange2(label.c_str(), &minVal, &maxVal, speed, min, max);

        if (ImGui::BeginPopupContextItem(label.c_str())) {

            if (ImGui::MenuItem("Expand range")) {
                minVal = min;
                maxVal = max;
            }
            if (ImGui::MenuItem("Expand to min")) {
                minVal = min;
            }
            if (ImGui::MenuItem("Expand to max")) {
                maxVal = max;
            }
            if (ImGui::MenuItem("Collapse to min")) {
                maxVal = minVal;
            }
            if (ImGui::MenuItem("Collapse to max")) {
                minVal = maxVal;
            }

            ImGui::EndPopup();
        }

        viz.setMod(mod);

        return py::make_tuple(minVal, maxVal);
    }, 
    py::arg("label"),
    py::arg("range"),
    py::arg("speed") = 1.0f,
    py::arg("min") = 0,
    py::arg("max") = 0);

    m.def("color_edit", [&](std::string label, array_like<float> color) {

        assert_shape(color, {{3}, {4}});

        bool mod = false;

        size_t colorSize = color.shape()[0];
        if (colorSize == 3) {
            mod = ImGui::ColorEdit3(label.c_str(), color.mutable_data());
        } else if (colorSize == 4) {
            mod = ImGui::ColorEdit4(label.c_str(), color.mutable_data());
        } else {
            throw std::runtime_error(
                    "Color must have 3 or 4 elements but has "
                    + std::to_string(colorSize) + ".");
        }

        viz.setMod(mod);

        return color;
    });

    m.def("image", [&](
                std::string id,
                py::array& image,
                int displayWidth,
                int displayHeight,
                array_like<double> tint,
                array_like<double> borderCol) {

        ImageInfo info = interpretImage(image);

        if (displayWidth < 0) {
            displayWidth = info.imageWidth;
        }
        if (displayHeight < 0) {
            displayHeight = info.imageHeight;
        }

        ImVec4 bc = interpretColor(borderCol);
        ImVec4 tn = interpretColor(tint);
        if (tn.w < 0) {
            tn = ImVec4(1, 1, 1, 1);
        }

        // calculate expected bounding box beforehand
        ImVec2 size(displayWidth, displayHeight);

        // essentially copied from ImGui::Image function
        ImGuiWindow* w = ImGui::GetCurrentWindow();
        ImRect bb(w->DC.CursorPos, w->DC.CursorPos + size);
        if (bc.w > 0.0f)
            bb.Max += ImVec2(2, 2);

        // upload to gpu

        GLuint textureId = 0;

        if (ImGui::IsRectVisible(bb.Min, bb.Max)) {
            // only upload the image to gpu, if it's actually visible
            // this improves performance for e.g. large lists of images
            textureId = uploadImage(id, info, image);
        }

        ImGui::Image((void*)(intptr_t)textureId,
                     size,
                     ImVec2(0, 0),
                     ImVec2(1, 1),
                     tn,
                     bc);
    },
    py::arg("id"),
    py::arg("image"),
    py::arg("width") = -1,
    py::arg("height") = -1,
    py::arg("tint") = py::array(),
    py::arg("border_col") = py::array());

    m.def("separator", ImGui::Separator);

    m.def("begin_tooltip", ImGui::BeginTooltip);
    m.def("end_tooltip", ImGui::EndTooltip);

    m.def("selectable", [&](std::string label, bool selected, ImVec2 size) { 

        bool s = ImGui::Selectable(label.c_str(), selected, 0, size);

        if (selected != s) {
            viz.setMod(true);
        }

        return s;
    },
    py::arg("label"),
    py::arg("selected"),
    py::arg("size") = ImVec2(0, 0));

    /**
     * Imgui style functions
     */

    m.def("style_colors_dark", [&](){
        ImGui::StyleColorsDark();
        ImPlot::StyleColorsDark();
    });

    m.def("style_colors_light", [&](){
        ImGui::StyleColorsLight();
        ImPlot::StyleColorsLight();
    });

    /**
     * Imgui layout functions
     */

    m.def("get_content_region_avail", ImGui::GetContentRegionAvail);

    m.def("get_viewport_center", [&]() { 
        return ImGui::GetMainViewport()->GetCenter();
    });

    m.def("same_line", []() {
        ImGui::SameLine();
    });

    /**
     * Imgui config helper functions
     */

    m.def("set_viewports_enable", [&](bool value) {

        ImGuiIO& io = ImGui::GetIO();

        if (value) {
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        } else {
            io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        }
    });

    m.def("set_ini_path", [&](std::string& path) {

        viz.iniFilePath = path;
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = viz.iniFilePath.c_str();
    });

    m.def("get_ini_path", []() {

        return std::string(ImGui::GetIO().IniFilename);
    });

    m.def("load_ini", [](std::string path) {

        ImGui::LoadIniSettingsFromDisk(path.c_str());
    },
    py::arg("path"));

    m.def("save_ini", [](std::string path) {

        ImGui::SaveIniSettingsToDisk(path.c_str());
    },
    py::arg("path"));

    /**
     * Imgui window helper functions
     */

    m.def("set_next_window_pos",
            ImGui::SetNextWindowPos,
    py::arg("position"),
    py::arg("cond") = ImGuiCond_None,
    py::arg("pivot") = py::array());

    m.def("set_next_window_size", 
            ImGui::SetNextWindowSize,
    py::arg("size"),
    py::arg("cond") = ImGuiCond_None);

    m.def("get_window_open", [&]() { 
        return viz.currentWindowOpen;
    });

    m.def("get_window_pos", ImGui::GetWindowPos);
    m.def("get_window_size", ImGui::GetWindowSize);

    m.def("want_capture_mouse", []() {
        ImGuiIO& io = ImGui::GetIO();
        return io.WantCaptureMouse;
    });

    m.def("want_capture_keyboard", []() {
        ImGuiIO& io = ImGui::GetIO();
        return io.WantCaptureKeyboard;
    });

    /**
     * Imgui item helper functions
     */

    m.def("set_next_item_width", 
            ImGui::SetNextItemWidth,
    py::arg("width"));

    m.def("set_next_item_open", 
            ImGui::SetNextItemOpen,
    py::arg("open"),
    py::arg("cond"));

    m.def("set_item_default_focus", ImGui::SetItemDefaultFocus);

    m.def("is_item_focused", ImGui::IsItemFocused);
    m.def("is_item_active", ImGui::IsItemActive);
    m.def("is_item_activated", ImGui::IsItemActivated);
    m.def("is_item_visible", ImGui::IsItemVisible);
    m.def("is_item_clicked", [&](int mouseButton) {
        return ImGui::IsItemClicked(mouseButton);
    },
    py::arg("mouse_button") = 0);

    m.def("is_item_hovered", [&]() { 
        return ImGui::IsItemHovered();
    });

    /**
     * Imgui ID management functions
     */

    m.def("push_id", [&](std::string id) {
        ImGui::PushID(id.c_str());
    },
    py::arg("id"));

    m.def("push_id", [&](int id) {
        ImGui::PushID(id);
    },
    py::arg("id"));

    m.def("push_override_id", [&](unsigned int id) {
        ImGui::PushOverrideID(id);
    },
    py::arg("id"));

    m.def("pop_id", ImGui::PopID);

    m.def("get_id", [&](std::string id) {
        return ImGui::GetID(id.c_str());
    },
    py::arg("id"));

    m.def("get_item_id", ImGui::GetItemID);

    /**
     * Drag'n'drop
     */

    m.def("begin_drag_drop_source", ImGui::BeginDragDropSource,
        py::arg("flags") = ImGuiDragDropFlags_None);
    m.def("end_drag_drop_source", ImGui::EndDragDropSource);

    m.def("begin_drag_drop_target", ImGui::BeginDragDropTarget);
    m.def("end_drag_drop_target", ImGui::EndDragDropTarget);

    m.def("set_drag_drop_payload", [](
                std::string id,
                py::object payload,
                ImGuiCond cond) {

        dragDropRef = payload;
        dragDropClearCounter += 2;

        int placeholder = 1;
        return ImGui::SetDragDropPayload(id.c_str(), &placeholder, sizeof(int), cond);
    },
    py::arg("id"),
    py::arg("payload"),
    py::arg("cond") = ImGuiCond_None);

    m.def("accept_drag_drop_payload", [](
                std::string id,
                ImGuiDragDropFlags flags) -> py::handle {

        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(id.c_str(), flags);

        if (payload) {
            return dragDropRef;
        }

        return py::object(py::none());
    },
    py::arg("id"),
    py::arg("flags") = ImGuiDragDropFlags_None);

    /**
     * DrawLists
     */

    m.def("get_window_drawlist",
            [&]() { return ImGui::GetCurrentWindow()->DrawList; },
            py::return_value_policy::reference);

    m.def("get_plot_drawlist",
            &ImPlot::GetPlotDrawList,
            py::return_value_policy::reference);

    m.def("push_clip_rect", [](ImVec2 pMin, ImVec2 pMax, bool intersect) {
        ImGui::PushClipRect(pMin, pMax, intersect);
    },
    py::arg("p_min"),
    py::arg("p_max"),
    py::arg("intersect") = false);

    m.def("pop_clip_rect", &ImGui::PopClipRect);

    m.def("push_plot_clip_rect", [](float expand) {
        ImPlot::PushPlotClipRect(expand);
    },
    py::arg("expand") = 0.0);

    m.def("pop_plot_clip_rect", &ImPlot::PopPlotClipRect);

    py::class_<ImDrawList>(m, "DrawList")
        .def("push_plot_transform", [&](ImDrawList& dl) {

            ImVec2 a = ImPlot::PlotToPixels(0.0, 0.0);
            ImVec2 b = ImPlot::PlotToPixels(10.0e7, 10.0e7);
            double scaleX = std::abs(((double)b.x - (double)a.x) / 10.0e7);
            double scaleY = std::abs(((double)b.y - (double)a.y) / 10.0e7);

            ImMatrix mat = ImMatrix::Scaling(scaleX, -scaleY);
            mat.m20 = a.x;
            mat.m21 = a.y;

            dl.PushTransformation(mat);
        })
        .def("push_transform", [&](ImDrawList& dl,
                                   ImVec2 trans,
                                   double rot,
                                   ImVec2 scale) {

            ImMatrix mat = ImMatrix::Combine(ImMatrix::Rotation(rot),
                                             ImMatrix::Scaling(scale.x, scale.y));
            mat.m20 = trans.x;
            mat.m21 = trans.y;

            dl.PushTransformation(mat);
        },
        py::arg("trans") = ImVec2(0.0, 0.0),
        py::arg("rot") = 0.0,
        py::arg("scale") = ImVec2(1.0, 1.0)
        )
        .def("pop_transform", [&](ImDrawList& dl) {
            dl.PopTransformation();
        })
        .def("add_line", [&](
                    ImDrawList& dl,
                    ImVec2& p1,
                    ImVec2& p2,
                    array_like<double> col,
                    float thickness){

            unsigned int startIndex = dl._VtxCurrentIdx;
            dl.AddLine(p1,
                       p2,
                       ImGui::GetColorU32(interpretColor(col)),
                       thickness);
            dl.ApplyTransformation(startIndex);
        },
        py::arg("p1"),
        py::arg("p2"),
        py::arg("col") = py::array(),
        py::arg("thickness") = 1.0
        )
        .def("add_polyline", [&](
                    ImDrawList& dl,
                    array_like<float> points,
                    array_like<double> col,
                    ImDrawFlags flags,
                    float thickness) {

            assert_shape(points, {{-1, 2}});

            unsigned int startIndex = dl._VtxCurrentIdx;
            dl.AddPolyline((ImVec2*)points.data(),
                           points.shape(0),
                           ImGui::GetColorU32(interpretColor(col)),
                           flags,
                           thickness);
            dl.ApplyTransformation(startIndex);
        },
        py::arg("points"),
        py::arg("col") = py::array(),
        py::arg("flags") = ImDrawFlags_None,
        py::arg("thickness") = 1.0
        )
        .def("add_rect", [&](
                    ImDrawList& dl,
                    ImVec2& pMin,
                    ImVec2& pMax,
                    array_like<double> col,
                    float rounding,
                    ImDrawFlags flags,
                    float thickness){

            unsigned int startIndex = dl._VtxCurrentIdx;
            dl.AddRect(
                    pMin,
                    pMax,
                    ImGui::GetColorU32(interpretColor(col)),
                    rounding,
                    flags,
                    thickness);
            dl.ApplyTransformation(startIndex);
        },
        py::arg("p_min"),
        py::arg("p_max"),
        py::arg("col") = py::array(),
        py::arg("rounding") = 0.0,
        py::arg("flags") = ImDrawFlags_None,
        py::arg("thickness") = 1.0
        )
        .def("add_rect_filled", [&](
                    ImDrawList& dl,
                    ImVec2& pMin,
                    ImVec2& pMax,
                    array_like<double> col,
                    float rounding,
                    ImDrawFlags flags) {

            unsigned int startIndex = dl._VtxCurrentIdx;
            dl.AddRectFilled(
                    pMin,
                    pMax,
                    ImGui::GetColorU32(interpretColor(col)),
                    rounding,
                    flags);
            dl.ApplyTransformation(startIndex);
        },
        py::arg("p_min"),
        py::arg("p_max"),
        py::arg("col"),
        py::arg("rounding") = 0.0,
        py::arg("flags") = ImDrawFlags_None
        )
        .def("add_rect_filled_multi_color", [&](
                    ImDrawList& dl,
                    ImVec2& pMin,
                    ImVec2& pMax,
                    array_like<double> ul,
                    array_like<double> ur,
                    array_like<double> br,
                    array_like<double> bl) {

            unsigned int startIndex = dl._VtxCurrentIdx;
            dl.AddRectFilledMultiColor(
                    pMin,
                    pMax,
                    ImGui::GetColorU32(interpretColor(ul)),
                    ImGui::GetColorU32(interpretColor(ur)),
                    ImGui::GetColorU32(interpretColor(br)),
                    ImGui::GetColorU32(interpretColor(bl))
                    );
            dl.ApplyTransformation(startIndex);
        },
        py::arg("p_min"),
        py::arg("p_max"),
        py::arg("col_up_left"),
        py::arg("col_up_right"),
        py::arg("col_bot_right"),
        py::arg("col_bot_left")
        )
        .def("add_quad", [&](
                    ImDrawList& dl,
                    array_like<double> points,
                    array_like<double> col,
                    float thickness){

            assert_shape(points, {{4, 2}});

            ImVec2 p0(points.at(0, 0), points.at(0, 1));
            ImVec2 p1(points.at(1, 0), points.at(1, 1));
            ImVec2 p2(points.at(2, 0), points.at(2, 1));
            ImVec2 p3(points.at(3, 0), points.at(3, 1));

            unsigned int startIndex = dl._VtxCurrentIdx;
            dl.AddQuad(
                    p0,
                    p1,
                    p2,
                    p3,
                    ImGui::GetColorU32(interpretColor(col)),
                    thickness);
            dl.ApplyTransformation(startIndex);
        },
        py::arg("points"),
        py::arg("col") = py::array(),
        py::arg("thickness") = 1.0
        )
        .def("add_quad_filled", [&](
                    ImDrawList& dl,
                    array_like<double> points,
                    array_like<double> col){

            assert_shape(points, {{4, 2}});

            ImVec2 p0(points.at(0, 0), points.at(0, 1));
            ImVec2 p1(points.at(1, 0), points.at(1, 1));
            ImVec2 p2(points.at(2, 0), points.at(2, 1));
            ImVec2 p3(points.at(3, 0), points.at(3, 1));

            unsigned int startIndex = dl._VtxCurrentIdx;
            dl.AddQuadFilled(
                    p0,
                    p1,
                    p2,
                    p3,
                    ImGui::GetColorU32(interpretColor(col)));
            dl.ApplyTransformation(startIndex);
        },
        py::arg("points"),
        py::arg("col") = py::array()
        )
        .def("add_circle", [&](
                    ImDrawList& dl,
                    ImVec2& center,
                    float radius,
                    array_like<double> col,
                    int numSegments,
                    float thickness){

            unsigned int startIndex = dl._VtxCurrentIdx;
            dl.AddCircle(
                    center,
                    radius,
                    ImGui::GetColorU32(interpretColor(col)),
                    numSegments,
                    thickness);
            dl.ApplyTransformation(startIndex);
        },
        py::arg("center"),
        py::arg("radius"),
        py::arg("col") = py::array(),
        py::arg("num_segments") = 36,
        py::arg("thickness") = 1.0
        )
        .def("add_circle_filled", [&](
                    ImDrawList& dl,
                    ImVec2& center,
                    float radius,
                    array_like<double> col,
                    int numSegments){

            unsigned int startIndex = dl._VtxCurrentIdx;
            dl.AddCircleFilled(
                    center,
                    radius,
                    ImGui::GetColorU32(interpretColor(col)),
                    numSegments);
            dl.ApplyTransformation(startIndex);
        },
        py::arg("center"),
        py::arg("radius"),
        py::arg("col") = py::array(),
        py::arg("num_segments") = 36)
        .def("add_text", [&](
                    ImDrawList& dl,
                    ImVec2 position,
                    std::string text,
                    array_like<double> col) {

            unsigned int startIndex = dl._VtxCurrentIdx;
            dl.AddText(position,
                       ImGui::GetColorU32(interpretColor(col)),
                       text.c_str());
            dl.ApplyTransformation(startIndex);
        },
        py::arg("position"),
        py::arg("text"),
        py::arg("col") = py::array());
}

void resetDragDrop() {

    if (dragDropClearCounter > 0) {
        dragDropClearCounter -= 1;
    } else {
        dragDropRef = py::none();
    }
}
