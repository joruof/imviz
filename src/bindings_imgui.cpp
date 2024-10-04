#include "bindings_implot.hpp"
#include "binding_helpers.hpp"
#include "imviz.hpp"

#define _USE_MATH_DEFINES
#include <cmath>
#include <sstream>
#include <iomanip>

#include <pybind11/pytypes.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "implot_internal.h"
#include "misc/cpp/imgui_stdlib.h"

static py::object dragDropRef = py::none();
static int dragDropClearCounter = 0;

namespace ImPlot {

    double PreciseAxisPlotToPixels (ImPlotAxis& axs, double plt) {

        if (axs.TransformForward != NULL) {
            double s = axs.TransformForward(plt, axs.TransformData);
            double t = (s - axs.ScaleMin) / (axs.ScaleMax - axs.ScaleMin);
            plt      = axs.Range.Min + axs.Range.Size() * t;
        }
        return (axs.PixelMin + axs.ScaleToPixel * (plt - axs.Range.Min));
    }

    ImPlotPoint PrecisePlotToPixels(double x, double y, ImAxis x_idx = IMPLOT_AUTO, ImAxis y_idx = IMPLOT_AUTO) {

        ImPlotContext& gp = *GImPlot;
        IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "PrecisePlotToPixels() needs to be called between BeginPlot() and EndPlot()!");
        IM_ASSERT_USER_ERROR(x_idx == IMPLOT_AUTO || (x_idx >= ImAxis_X1 && x_idx < ImAxis_Y1),    "X-Axis index out of bounds!");
        IM_ASSERT_USER_ERROR(y_idx == IMPLOT_AUTO || (y_idx >= ImAxis_Y1 && y_idx < ImAxis_COUNT), "Y-Axis index out of bounds!");

        SetupLock();

        ImPlotPlot& plot = *gp.CurrentPlot;
        ImPlotAxis& x_axis = x_idx == IMPLOT_AUTO ? plot.Axes[plot.CurrentX] : plot.Axes[x_idx];
        ImPlotAxis& y_axis = y_idx == IMPLOT_AUTO ? plot.Axes[plot.CurrentY] : plot.Axes[y_idx];

        return ImPlotPoint(PreciseAxisPlotToPixels(x_axis, x),
                           PreciseAxisPlotToPixels(y_axis, y));
    }
}

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

    py::enum_<ImGuiDir_>(m, "Dir")
        .value("NONE", ImGuiDir_None)
        .value("LEFT", ImGuiDir_Left)
        .value("RIGHT", ImGuiDir_Right)
        .value("UP", ImGuiDir_Up)
        .value("DOWN", ImGuiDir_Down);

    py::enum_<ImGuiWindowFlags_>(m, "WindowFlags")
        .value("NONE", ImGuiWindowFlags_None)
        .value("NO_TITLE_BAR", ImGuiWindowFlags_NoTitleBar)
        .value("NO_RESIZE", ImGuiWindowFlags_NoResize)
        .value("NO_MOVE", ImGuiWindowFlags_NoMove)
        .value("NO_SCROLL_BAR", ImGuiWindowFlags_NoScrollbar)
        .value("NO_SCROLL_WITH_MOUSE", ImGuiWindowFlags_NoScrollWithMouse)
        .value("NO_COLLAPSE", ImGuiWindowFlags_NoCollapse)
        .value("ALWAYS_AUTO_RESIZE", ImGuiWindowFlags_AlwaysAutoResize)
        .value("NO_BACKGROUND", ImGuiWindowFlags_NoBackground)
        .value("NO_SAVED_SETTINGS", ImGuiWindowFlags_NoSavedSettings)
        .value("NO_MOUSE_INPUTS", ImGuiWindowFlags_NoMouseInputs)
        .value("MENUBAR", ImGuiWindowFlags_MenuBar)
        .value("HORIZONTAL_SCROLLBAR", ImGuiWindowFlags_HorizontalScrollbar)
        .value("NO_FOCUS_ON_APPEARING", ImGuiWindowFlags_NoFocusOnAppearing)
        .value("NO_BRING_TO_FRONT_ON_FOCUS", ImGuiWindowFlags_NoBringToFrontOnFocus)
        .value("ALWAYS_VERTICAL_SCROLLBAR", ImGuiWindowFlags_AlwaysVerticalScrollbar)
        .value("ALWAYS_HORIZONTAL_SCROLLBAR", ImGuiWindowFlags_AlwaysHorizontalScrollbar)
        .value("ALWAYS_USE_WINDOW_PADDING", ImGuiWindowFlags_AlwaysUseWindowPadding)
        .value("NO_NAV_INPUTS", ImGuiWindowFlags_NoNavInputs)
        .value("NO_NAV_FOCUS", ImGuiWindowFlags_NoNavFocus)
        .value("UNSAVED_DOCUMENT", ImGuiWindowFlags_UnsavedDocument)
        .value("NO_DOCKING", ImGuiWindowFlags_NoDocking)
        .value("NO_NAV", ImGuiWindowFlags_NoNav)
        .value("NO_DECORATION", ImGuiWindowFlags_NoDecoration)
        .value("NO_INPUTS", ImGuiWindowFlags_NoInputs)
        .value("NAV_FLATTENED", ImGuiWindowFlags_NavFlattened)
        .value("CHILD_WINDOW", ImGuiWindowFlags_ChildWindow)
        .value("TOOLTIP", ImGuiWindowFlags_Tooltip)
        .value("POPUP", ImGuiWindowFlags_Popup)
        .value("MODAL", ImGuiWindowFlags_Modal)
        .value("CHILD_MENU", ImGuiWindowFlags_ChildMenu)
        .value("DOCK_NODE_HOST", ImGuiWindowFlags_DockNodeHost);

    py::enum_<ImGuiTreeNodeFlags_>(m, "TreeNodeFlags", py::arithmetic())
        .value("NONE", ImGuiTreeNodeFlags_None)
        .value("SELECTED", ImGuiTreeNodeFlags_Selected)
        .value("FRAMED", ImGuiTreeNodeFlags_Framed)
        .value("ALLOW_ITEM_OVERLAP", ImGuiTreeNodeFlags_AllowItemOverlap)
        .value("NO_TREE_PUSH_ON_OPEN", ImGuiTreeNodeFlags_NoTreePushOnOpen)
        .value("NO_AUTO_OPEN_NO_LOG", ImGuiTreeNodeFlags_NoAutoOpenOnLog)
        .value("DEFAULT_OPEN", ImGuiTreeNodeFlags_DefaultOpen)
        .value("OPEN_ON_DOUBLE_CLICK", ImGuiTreeNodeFlags_OpenOnDoubleClick)
        .value("OPEN_ON_ARROW", ImGuiTreeNodeFlags_OpenOnArrow)
        .value("LEAF", ImGuiTreeNodeFlags_Leaf)
        .value("BULLET", ImGuiTreeNodeFlags_Bullet)
        .value("FRAME_PADDING", ImGuiTreeNodeFlags_FramePadding)
        .value("SPAN_AVAIL_WIDTH", ImGuiTreeNodeFlags_SpanAvailWidth)
        .value("SPAN_FULL_WIDTH", ImGuiTreeNodeFlags_SpanFullWidth)
        .value("NAV_LEFT_JUMPS_BACK_HERE", ImGuiTreeNodeFlags_NavLeftJumpsBackHere)
        .value("COLLAPSING_HEADER", ImGuiTreeNodeFlags_CollapsingHeader);

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

    py::enum_<ImDrawFlags_>(m, "DrawFlags", py::arithmetic())
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

    py::enum_<ImGuiTableFlags_>(m, "TableFlags", py::arithmetic())
        .value("NONE", ImGuiTableFlags_None)
        .value("RESIZABLE", ImGuiTableFlags_Resizable)
        .value("REORDERABLE", ImGuiTableFlags_Reorderable)
        .value("HIDEABLE", ImGuiTableFlags_Hideable)
        .value("SORTABLE", ImGuiTableFlags_Sortable)
        .value("NO_SAVED_SETTINGS", ImGuiTableFlags_NoSavedSettings)
        .value("CONTEXT_MENU_IN_BODY", ImGuiTableFlags_ContextMenuInBody)
        .value("ROWBG", ImGuiTableFlags_RowBg)
        .value("BORDERS_INNER_H", ImGuiTableFlags_BordersInnerH)
        .value("BORDERS_OUTER_H", ImGuiTableFlags_BordersOuterH)
        .value("BORDERS_INNER_V", ImGuiTableFlags_BordersInnerV)
        .value("BORDERS_OUTER_V", ImGuiTableFlags_BordersOuterV)
        .value("BORDERS_H", ImGuiTableFlags_BordersH)
        .value("BORDERS_V", ImGuiTableFlags_BordersV)
        .value("BORDERS_INNER", ImGuiTableFlags_BordersInner)
        .value("BORDERS_OUTER", ImGuiTableFlags_BordersOuter)
        .value("BORDERS", ImGuiTableFlags_Borders)
        .value("NO_BORDERS_IN_BODY", ImGuiTableFlags_NoBordersInBody)
        .value("NO_BORDERS_IN_BODY_UNTIL_RESIZE", ImGuiTableFlags_NoBordersInBodyUntilResize)
        .value("SIZING_FIXED_FIT", ImGuiTableFlags_SizingFixedFit)
        .value("SIZING_FIXED_SAME", ImGuiTableFlags_SizingFixedSame)
        .value("SIZING_STRETCH_PROP", ImGuiTableFlags_SizingStretchProp)
        .value("SIZING_STRETCH_SAME", ImGuiTableFlags_SizingStretchSame)
        .value("NO_HOST_EXTEND_X", ImGuiTableFlags_NoHostExtendX)
        .value("NO_HOST_EXTEND_Y", ImGuiTableFlags_NoHostExtendY)
        .value("NO_KEEP_COLUMNS_VISIBLE", ImGuiTableFlags_NoKeepColumnsVisible)
        .value("PRECISE_WIDTHS", ImGuiTableFlags_PreciseWidths)
        .value("NO_CLIP", ImGuiTableFlags_NoClip)
        .value("PAD_OUTER_X", ImGuiTableFlags_PadOuterX)
        .value("NO_PAD_OUTER_X", ImGuiTableFlags_NoPadOuterX)
        .value("NO_PAD_INNER_X", ImGuiTableFlags_NoPadInnerX)
        .value("SCROLL_X", ImGuiTableFlags_ScrollX)
        .value("SCROLL_Y", ImGuiTableFlags_ScrollY)
        .value("SORT_MULTI", ImGuiTableFlags_SortMulti)
        .value("SORT_TRISTATE", ImGuiTableFlags_SortTristate)
        .value("SIZINGMASK_", ImGuiTableFlags_SizingMask_);

    py::enum_<ImGuiTableColumnFlags_>(m, "TableColumnFlags", py::arithmetic())
        .value("NONE", ImGuiTableColumnFlags_None)
        .value("DISABLED", ImGuiTableColumnFlags_Disabled)
        .value("DEFAULT_HIDE", ImGuiTableColumnFlags_DefaultHide)
        .value("DEFAULT_SORT", ImGuiTableColumnFlags_DefaultSort)
        .value("WIDTH_STRETCH", ImGuiTableColumnFlags_WidthStretch)
        .value("WIDTH_FIXED", ImGuiTableColumnFlags_WidthFixed)
        .value("NO_RESIZE", ImGuiTableColumnFlags_NoResize)
        .value("NO_REORDER", ImGuiTableColumnFlags_NoReorder)
        .value("NO_HIDE", ImGuiTableColumnFlags_NoHide)
        .value("NO_CLIP", ImGuiTableColumnFlags_NoClip)
        .value("NO_SORT", ImGuiTableColumnFlags_NoSort)
        .value("NO_SORT_ASCENDING", ImGuiTableColumnFlags_NoSortAscending)
        .value("NO_SORT_DESCENDING", ImGuiTableColumnFlags_NoSortDescending)
        .value("NO_HEADER_LABEL", ImGuiTableColumnFlags_NoHeaderLabel)
        .value("NO_HEADER_WIDTH", ImGuiTableColumnFlags_NoHeaderWidth)
        .value("PREFER_SORT_ASCENDING", ImGuiTableColumnFlags_PreferSortAscending)
        .value("PREFER_SORT_DESCENDING", ImGuiTableColumnFlags_PreferSortDescending)
        .value("INDENT_ENABLE", ImGuiTableColumnFlags_IndentEnable)
        .value("INDENT_DISABLE", ImGuiTableColumnFlags_IndentDisable)
        .value("IS_ENABLED", ImGuiTableColumnFlags_IsEnabled)
        .value("IS_VISIBLE", ImGuiTableColumnFlags_IsVisible)
        .value("IS_SORTED", ImGuiTableColumnFlags_IsSorted)
        .value("IS_HOVERED", ImGuiTableColumnFlags_IsHovered)
        .value("WIDTH_MASK_", ImGuiTableColumnFlags_WidthMask_)
        .value("INDENT_MASK_", ImGuiTableColumnFlags_IndentMask_)
        .value("STATUS_MASK_", ImGuiTableColumnFlags_StatusMask_)
        .value("NO_DIRECT_RESIZE_", ImGuiTableColumnFlags_NoDirectResize_);

    py::enum_<ImGuiTableRowFlags_>(m, "TableRowFlags", py::arithmetic())
        .value("NONE", ImGuiTableRowFlags_None)
        .value("HEADERS", ImGuiTableRowFlags_Headers);

    py::enum_<ImGuiTableBgTarget_>(m, "TableBgTarget")
        .value("NONE", ImGuiTableBgTarget_None)
        .value("ROW_BG_0", ImGuiTableBgTarget_RowBg0)
        .value("ROW_BG_1", ImGuiTableBgTarget_RowBg1)
        .value("CELL_BG", ImGuiTableBgTarget_CellBg);

    py::enum_<ImGuiDockNodeFlags_>(m, "DockNodeFlags")
        .value("NONE", ImGuiDockNodeFlags_None)
        .value("KEEP_ALIVE_ONLY", ImGuiDockNodeFlags_KeepAliveOnly)
        .value("NO_DOCKING_IN_CENTRAL_NODE", ImGuiDockNodeFlags_NoDockingInCentralNode)
        .value("PASSTHRU_CENTRAL_NODE", ImGuiDockNodeFlags_PassthruCentralNode)
        .value("NO_SPLIT", ImGuiDockNodeFlags_NoSplit)
        .value("NO_RESIZE", ImGuiDockNodeFlags_NoResize)
        .value("AUTO_HIDE_TAB_BAR", ImGuiDockNodeFlags_AutoHideTabBar);

    py::enum_<ImGuiCol_>(m, "GuiCol")
        .value("TEXT", ImGuiCol_Text)
        .value("TEXT_DISABLED", ImGuiCol_TextDisabled)
        .value("WINDOW_BG", ImGuiCol_WindowBg)
        .value("CHILD_BG", ImGuiCol_ChildBg)
        .value("POPUP_BG", ImGuiCol_PopupBg)
        .value("BORDER", ImGuiCol_Border)
        .value("BORDER_SHADOW", ImGuiCol_BorderShadow)
        .value("FRAME_BG", ImGuiCol_FrameBg)
        .value("FRAME_BG_HOVERED", ImGuiCol_FrameBgHovered)
        .value("FRAME_BG_ACTIVE", ImGuiCol_FrameBgActive)
        .value("TITLE_BG", ImGuiCol_TitleBg)
        .value("TITLE_BG_ACTIVE", ImGuiCol_TitleBgActive)
        .value("TITLE_BG_COLLAPSED", ImGuiCol_TitleBgCollapsed)
        .value("MENU_BAR_BG", ImGuiCol_MenuBarBg)
        .value("SCROLL_BAR_BG", ImGuiCol_ScrollbarBg)
        .value("SCROLL_BAR_GRAB", ImGuiCol_ScrollbarGrab)
        .value("SCROLL_BAR_GRAB_HOVERED", ImGuiCol_ScrollbarGrabHovered)
        .value("SCROLL_BAR_GRAB_ACTIVE", ImGuiCol_ScrollbarGrabActive)
        .value("CHECK_MARK", ImGuiCol_CheckMark)
        .value("SLIDER_GRAB", ImGuiCol_SliderGrab)
        .value("SLIDER_GRAB_ACTIVE", ImGuiCol_SliderGrabActive)
        .value("BUTTON", ImGuiCol_Button)
        .value("BUTTON_HOVERED", ImGuiCol_ButtonHovered)
        .value("BUTTON_ACTIVE", ImGuiCol_ButtonActive)
        .value("HEADER", ImGuiCol_Header)
        .value("HEADER_HOVERED", ImGuiCol_HeaderHovered)
        .value("HEADER_ACTIVE", ImGuiCol_HeaderActive)
        .value("SEPARATOR", ImGuiCol_Separator)
        .value("SEPARATOR_HOVERED", ImGuiCol_SeparatorHovered)
        .value("SEPARATOR_ACTIVE", ImGuiCol_SeparatorActive)
        .value("RESIZE_GRIP", ImGuiCol_ResizeGrip)
        .value("RESIZE_GRIP_HOVERED", ImGuiCol_ResizeGripHovered)
        .value("RESIZE_GRIP_ACTIVE", ImGuiCol_ResizeGripActive)
        .value("TAB", ImGuiCol_Tab)
        .value("TAB_HOVERED", ImGuiCol_TabHovered)
        .value("TAB_ACTIVE", ImGuiCol_TabActive)
        .value("TAB_UNFOCUSED", ImGuiCol_TabUnfocused)
        .value("TAB_UNFOCUSED_ACTIVE", ImGuiCol_TabUnfocusedActive)
        .value("DOCKING_PREVIEW", ImGuiCol_DockingPreview)
        .value("DOCKING_EMPTY_BG", ImGuiCol_DockingEmptyBg)
        .value("PLOT_LINES", ImGuiCol_PlotLines)
        .value("PLOT_LINES_HOVERED", ImGuiCol_PlotLinesHovered)
        .value("PLOT_HISTOGRAM", ImGuiCol_PlotHistogram)
        .value("PLOT_HISTOGRAM_HOVERED", ImGuiCol_PlotHistogramHovered)
        .value("TABLE_HEADER_BG", ImGuiCol_TableHeaderBg)
        .value("TABLE_BORDER_STRONG", ImGuiCol_TableBorderStrong)
        .value("TABLE_BORDER_LIGHT", ImGuiCol_TableBorderLight)
        .value("TABLE_ROW_BG", ImGuiCol_TableRowBg)
        .value("TABLE_ROW_BG_ALT", ImGuiCol_TableRowBgAlt)
        .value("TEXT_SELECTED_BG", ImGuiCol_TextSelectedBg)
        .value("DRAG_DROP_TARGET", ImGuiCol_DragDropTarget)
        .value("NAV_HIGHLIGHT", ImGuiCol_NavHighlight)
        .value("NAV_WINDOWING_HIGHLIGHT", ImGuiCol_NavWindowingHighlight)
        .value("NAV_WINDOWING_DIM_BG", ImGuiCol_NavWindowingDimBg)
        .value("MODAL_WINDOW_DIM_BG", ImGuiCol_ModalWindowDimBg);

    py::enum_<ImGuiStyleVar_>(m, "StyleVar")
        .value("ALPHA", ImGuiStyleVar_Alpha)
        .value("DISABLED_ALPHA", ImGuiStyleVar_DisabledAlpha)
        .value("WINDOW_PADDING", ImGuiStyleVar_WindowPadding)
        .value("WINDOW_ROUNDING", ImGuiStyleVar_WindowRounding)
        .value("WINDOW_BORDER_SIZE", ImGuiStyleVar_WindowBorderSize)
        .value("WINDOW_MIN_SIZE", ImGuiStyleVar_WindowMinSize)
        .value("WINDOW_TITLE_ALIGN", ImGuiStyleVar_WindowTitleAlign)
        .value("CHILD_ROUNDING", ImGuiStyleVar_ChildRounding)
        .value("CHILD_BORDER_SIZE", ImGuiStyleVar_ChildBorderSize)
        .value("POPUP_ROUNDING", ImGuiStyleVar_PopupRounding)
        .value("POPUP_BORDER_SIZE", ImGuiStyleVar_PopupBorderSize)
        .value("FRAME_PADDING", ImGuiStyleVar_FramePadding)
        .value("FRAME_ROUNDING", ImGuiStyleVar_FrameRounding)
        .value("FRAME_BORDER_SIZE", ImGuiStyleVar_FrameBorderSize)
        .value("ITEM_SPACING", ImGuiStyleVar_ItemSpacing)
        .value("ITEM_INNER_SPACING", ImGuiStyleVar_ItemInnerSpacing)
        .value("INDENT_SPACING", ImGuiStyleVar_IndentSpacing)
        .value("CELL_PADDING", ImGuiStyleVar_CellPadding)
        .value("SCROLL_BAR_SIZE", ImGuiStyleVar_ScrollbarSize)
        .value("SCROLL_BAR_ROUNDING", ImGuiStyleVar_ScrollbarRounding)
        .value("GRAB_MIN_SIZE", ImGuiStyleVar_GrabMinSize)
        .value("GRAB_ROUNDING", ImGuiStyleVar_GrabRounding)
        .value("TAB_ROUNDING", ImGuiStyleVar_TabRounding)
        .value("BUTTON_TEXT_ALIGN", ImGuiStyleVar_ButtonTextAlign)
        .value("SELECTABLE_TEXT_ALIGN", ImGuiStyleVar_SelectableTextAlign);

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
                       bool autoResize,
                       bool menuBar) {

        if (position.shape()[0] > 0) { 
            assert_shape(position, {{2}});
            const float* data = position.data();
            ImGui::SetNextWindowPos({data[0], data[1]});
        }
        if (size.shape()[0] > 0) {
            assert_shape(size, {{2}});
            const float* data = size.data();
            ImGui::SetNextWindowSize({data[0], data[1]});
        } else {
            ImGui::SetNextWindowSize(ImVec2(320, 240), ImGuiCond_Once);
        }

        ImGuiWindowFlags flags = ImGuiWindowFlags_None;

        flags |= ImGuiWindowFlags_NoTitleBar * !title_bar;
        flags |= ImGuiWindowFlags_NoResize * !resize;
        flags |= ImGuiWindowFlags_NoMove * !move;
        flags |= ImGuiWindowFlags_NoScrollbar * !scrollbar;
        flags |= ImGuiWindowFlags_NoScrollWithMouse * !scrollWithMouse;
        flags |= ImGuiWindowFlags_NoCollapse * !collapse;
        flags |= ImGuiWindowFlags_AlwaysAutoResize * autoResize;
        flags |= ImGuiWindowFlags_MenuBar * menuBar;

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
    py::arg("auto_resize") = false,
    py::arg("menu_bar") = false);

    m.def("end_window", ImGui::End);

    m.def("begin_child", [](std::string label, ImVec2 size, bool border, ImGuiWindowFlags flags){

        return ImGui::BeginChild(label.c_str(), size, border, flags);
    },
    py::arg("label"),
    py::arg("size") = ImVec2(0.0f, 0.0f),
    py::arg("border") = false,
    py::arg("flags") = ImGuiWindowFlags_None);

    m.def("end_child", ImGui::EndChild);

    m.def("set_scroll_here_x", ImGui::SetScrollHereX);
    m.def("set_scroll_here_y", ImGui::SetScrollHereY);

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

    m.def("tree_node", [&](std::string label, ImGuiTreeNodeFlags flags) {
        return ImGui::TreeNodeEx(label.c_str(), flags);
    },
    py::arg("label") = "",
    py::arg("flags") = ImGuiTreeNodeFlags_None);

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

    m.def("text", [&](py::handle obj, py::handle color) {

        ImVec4 c = interpretColor(color);

        std::string str = py::str(obj);

        if (c.w >= 0) {
            ImGui::TextColored(c, "%s", str.c_str());
        } else {
            ImGui::Text("%s", str.c_str());
        }
    },
    py::arg("str"),
    py::arg("color") = py::array());

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

    m.def("get_input_cursor_index", [&](std::string label) {

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (nullptr == window) {
            return -1;
        }

        const ImGuiID id = window->GetID(label.c_str());

        ImGuiInputTextState* state = ImGui::GetInputTextState(id);
        if (nullptr == state) {
            return -1;
        }

        return state->Stb.cursor;
    },
    py::arg("label"));

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

    m.def("slider_int", [&](const std::string title, int& value, const int min, const int max) {
        
        bool mod = ImGui::SliderInt(
                title.c_str(), &value, min, max);
        viz.setMod(mod);

        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
    py::arg("min") = 0,
    py::arg("max") = 1);

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

    m.def("image_texture", [&](
                GLuint textureId,
                ImVec2 size,
                array_like<double> tint,
                array_like<double> borderCol) {

        ImVec4 bc = interpretColor(borderCol);
        ImVec4 tn = interpretColor(tint);
        if (tn.w < 0) {
            tn = ImVec4(1, 1, 1, 1);
        }

        ImGui::Image((void*)(intptr_t)textureId,
                     size,
                     ImVec2(0, 0),
                     ImVec2(1, 1),
                     tn,
                     bc);
    },
    py::arg("texture_id"),
    py::arg("size"),
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
     * Tables & Columns
     */

    m.def("begin_table", [&](std::string label,
                             int columns,
                             ImGuiTableFlags flags,
                             ImVec2 outerSize,
                             float innerWidth) {

        return ImGui::BeginTable(label.c_str(), columns, flags, outerSize, innerWidth);
    },
    py::arg("label"),
    py::arg("columns"),
    py::arg("flags") = ImGuiTableFlags_None,
    py::arg("outer_size") = ImVec2(0.0f, 0.0f),
    py::arg("inner_width") = 0.0f);

    m.def("end_table", &ImGui::EndTable);

    m.def("table_next_row", [&](ImGuiTableRowFlags flags, float minRowHeight) {

        ImGui::TableNextRow(flags, minRowHeight);
    },
    py::arg("flags") = ImGuiTableRowFlags_None,
    py::arg("min_row_height") = 0.0f);

    m.def("table_next_column", &ImGui::TableNextColumn);
    m.def("table_set_column_index", &ImGui::TableSetColumnIndex);

    m.def("table_setup_column", [&](std::string label,
                                    ImGuiTableColumnFlags flags,
                                    float initWidthOrWeight){
        ImGui::TableSetupColumn(label.c_str(), flags, initWidthOrWeight);
    },
    py::arg("label"),
    py::arg("flags") = ImGuiTableColumnFlags_None,
    py::arg("init_width_or_weight") = 0.0f
    );

    m.def("table_headers_row", &ImGui::TableHeadersRow);

    m.def("table_setup_scroll_freeze", &ImGui::TableSetupScrollFreeze);

    /**
     * Imgui style functions
     */

    m.def("style_colors_dark", [&](){
        ImGui::StyleColorsDark();
        ImPlot::StyleColorsDark();
    });

    m.def("style_colors_imviz", [&](){

        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
        colors[ImGuiCol_Border]                 = ImVec4(0.215f, 0.215f, 0.215f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.39f, 0.39f, 0.39f, 0.67f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.27f, 0.27f, 0.27f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.27f, 0.27f, 0.27f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.39f, 0.39f, 0.39f, 0.95f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.20f, 0.20f, 0.20f, 0.86f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.69f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = 3;
        style.ChildRounding = 0;
        style.FrameRounding = 2;
        style.PopupRounding = 3;
        style.ScrollbarRounding = 9;
        style.GrabRounding = 2;
        style.TabRounding = 2;
        style.FrameBorderSize = 1.0f;
        style.TabBorderSize = 1.0f;
        style.ScrollbarSize = 15.0f;
        style.IndentSpacing = 19.0f;
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

    m.def("get_main_dockspace_id", [&](){ return viz.mainDockSpaceId; });

    m.def("dock_builder_add_node", [&](ImGuiID nodeId, ImGuiDockNodeFlags flags){
        return ImGui::DockBuilderAddNode(nodeId, flags);
    },
    py::arg("node_id") = 0,
    py::arg("flags") = ImGuiDockNodeFlags_None);

    m.def("dock_builder_split_node", [&](ImGuiID nodeId, ImGuiDir splitDir, float ratio){

        ImGuiID node_a = 0;
        ImGuiID node_b = 0;
        ImGui::DockBuilderSplitNode(nodeId, splitDir, ratio, &node_a, &node_b);

        return py::make_tuple(node_a, node_b);
    },
    py::arg("nodeId") = 0,
    py::arg("split_dir"),
    py::arg("ratio"));

    m.def("dock_builder_dock_window", [](std::string windowName, ImGuiID nodeId) {
        ImGui::DockBuilderDockWindow(windowName.c_str(), nodeId);
    },
    py::arg("window_name"),
    py::arg("node_id"));

    m.def("dock_builder_set_node_size", [](ImGuiID nodeId, ImVec2 size) {
              ImGui::DockBuilderSetNodeSize(nodeId, size);
          },
    py::arg("node_id"),
    py::arg("size"));

    m.def("dock_builder_remove_node", ImGui::DockBuilderRemoveNode);
    m.def("dock_builder_remove_node_child_nodes", ImGui::DockBuilderRemoveNodeChildNodes);
    m.def("dock_builder_finish", ImGui::DockBuilderFinish);

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

    m.def("load_ini_from_str", [](std::string ini) {

        ImGui::LoadIniSettingsFromMemory(ini.c_str(), ini.size());
    },
    py::arg("ini"));

    m.def("save_ini", [](std::string path) {

        ImGui::SaveIniSettingsToDisk(path.c_str());
    },
    py::arg("path"));

    m.def("save_ini_to_str", []() {

        size_t outSize = 0;
        const char* chars = ImGui::SaveIniSettingsToMemory(&outSize);
        return std::string(chars, outSize);
    });

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

    m.def("set_next_window_size_contraints", [](ImVec2 minSize, ImVec2 maxSize) {
        ImGui::SetNextWindowSizeConstraints(minSize, maxSize);
    },
    py::arg("min_size"),
    py::arg("max_size"));

    m.def("get_window_open", [&]() { 
        return viz.currentWindowOpen;
    });

    m.def("get_window_pos", ImGui::GetWindowPos);
    m.def("get_window_size", ImGui::GetWindowSize);

    m.def("is_window_focused", [](){
        return ImGui::IsWindowFocused();
    });
    m.def("is_window_hovered", [](){
        return ImGui::IsWindowHovered();
    });
    m.def("is_window_docked", ImGui::IsWindowDocked);
    m.def("is_window_appearing", ImGui::IsWindowAppearing);
    m.def("is_window_collapsed", ImGui::IsWindowCollapsed);

    m.def("want_capture_mouse", []() {
        ImGuiIO& io = ImGui::GetIO();
        return io.WantCaptureMouse;
    });

    m.def("want_capture_keyboard", []() {
        ImGuiIO& io = ImGui::GetIO();
        return io.WantCaptureKeyboard;
    });

    /**
     * Font functions
     */

    m.def("get_global_font_size", [&]() {
        return viz.smallFont->FontSize;
    });

    m.def("set_global_font_size", [&](double baseSize) {
        return viz.fontBaseSize = baseSize;
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
    m.def("is_item_deactivated_after_edit", ImGui::IsItemDeactivatedAfterEdit);
    m.def("is_item_deactivated", ImGui::IsItemDeactivated);
    m.def("is_item_visible", ImGui::IsItemVisible);
    m.def("is_item_toggled_open", ImGui::IsItemToggledOpen);
    m.def("is_item_toggled_selection", ImGui::IsItemToggledSelection);
    m.def("is_item_edited", ImGui::IsItemEdited);
    m.def("is_item_clicked", [&](int mouseButton) {
        return ImGui::IsItemClicked(mouseButton);
    },
    py::arg("mouse_button") = 0);
    m.def("is_item_hovered", [&]() { 
        return ImGui::IsItemHovered();
    });

    m.def("is_mouse_clicked", [](int mouseButton) {
        return ImGui::IsMouseClicked(mouseButton);
    },
    py::arg("mouse_button") = 0);

    m.def("is_mouse_double_clicked", [](int mouseButton) {
        return ImGui::IsMouseDoubleClicked(mouseButton);
    },
    py::arg("mouse_button") = 0);

    m.def("get_mouse_drag_delta", [](int mouseButton, float lockThreshold) {
        return ImGui::GetMouseDragDelta(mouseButton, lockThreshold);
    },
    py::arg("mouse_button") = 0,
    py::arg("lock_threshold") = -1.0f);

    m.def("reset_mouse_drag_delta", [](int mouseButton) {
        ImGui::ResetMouseDragDelta(mouseButton);
    },
    py::arg("mouse_button") = 0);

    m.def("begin_disabled", [](bool disabled) {
        ImGui::BeginDisabled(disabled);
    },
    py::arg("disabled") = true);

    m.def("end_disabled", &ImGui::EndDisabled);

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
     * Styling
     */

    m.def("push_style_color", [](ImGuiCol idx, py::handle& col){
        ImVec4 color = interpretColor(col); 
        ImGui::PushStyleColor(idx, color);
    },
    py::arg("idx"),
    py::arg("col"));

    m.def("pop_style_color", [](int count) {
        ImGui::PopStyleColor(count);
    }, 
    py::arg("count") = 1);

    m.def("push_style_var", [](ImGuiStyleVar idx, ImVec2 val){
        ImGui::PushStyleVar(idx, val);
    },
    py::arg("idx"),
    py::arg("val"));

    m.def("pop_style_var", [](int count) {
        ImGui::PopStyleVar(count);
    }, 
    py::arg("count") = 1);

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

    struct VizMatrix
    {
        // Taken from this great patch: 
        // https://github.com/ocornut/imgui/compare/master...thedmd:feature/draw-list-transformation

        double m00, m01, m10, m11, m20, m21;
        VizMatrix() { m00 = m11 = 1.0; m01 = m10 = m20 = m21 = 0.0; }
        VizMatrix(double _m00, double _m01, double _m10, double _m11, double _m20, double _m21)
        { m00 = _m00; m01 = _m01; m10 = _m10; m11 = _m11; m20 = _m20; m21 = _m21; }
#ifdef IM_MATRIX_CLASS_EXTRA // Define constructor and implicit cast operators in imconfig.h to convert back<>forth from your math types and VizMatrix.
        IM_MATRIX_CLASS_EXTRA
#endif

        VizMatrix Inverted() const {

            const float d00 = m11;
            const float d01 = m01;

            const float d10 = m10;
            const float d11 = m00;

            const float d20 = m10 * m21 - m11 * m20;
            const float d21 = m00 * m21 - m01 * m20;

            const float d = m00 * d00 - m10 * d01;

            const float invD = d ? 1.0f / d : 0.0f;

            return VizMatrix(
                 d00 * invD, -d01 * invD,
                -d10 * invD,  d11 * invD,
                 d20 * invD, -d21 * invD);
        }

        static inline VizMatrix Translation(const ImVec2& p) { return Translation(p.x, p.y); }
        static inline VizMatrix Translation(double x, double y) { return VizMatrix(1.0, 0.0, 0.0, 1.0, x, y); }
        static inline VizMatrix Scaling(const ImVec2& p) { return Scaling(p.x, p.y); }
        static inline VizMatrix Scaling(double x, double y) { return VizMatrix(x, 0.0, 0.0, y, 0.0, 0.0); }
        static inline VizMatrix Shear(const ImVec2& p) { return Shear(p.x, p.y); }
        static inline VizMatrix Shear(double x, double y) { return VizMatrix(1.0, y, x, 1.0, 0.0, 0.0); }
        IMGUI_API static VizMatrix Rotation(double angle) {
            const double s = sin(angle);
            const double c = cos(angle);
            return VizMatrix(c, s, -s, c, 0.0, 0.0);
        }

        static inline VizMatrix Combine(const VizMatrix& lhs, const VizMatrix& rhs) // lhs * rhs = out
        {
            return VizMatrix(
                rhs.m00 * lhs.m00 + rhs.m10 * lhs.m01,
                rhs.m01 * lhs.m00 + rhs.m11 * lhs.m01,
                rhs.m00 * lhs.m10 + rhs.m10 * lhs.m11,
                rhs.m01 * lhs.m10 + rhs.m11 * lhs.m11,
                rhs.m00 * lhs.m20 + rhs.m10 * lhs.m21 + rhs.m20,
                rhs.m01 * lhs.m20 + rhs.m11 * lhs.m21 + rhs.m21);
        }
        inline void Transform(ImVec2* v, size_t count = 1) const
        {
            for (size_t i = 0; i < count; ++i, ++v)
            {
                *v = ImVec2(
                    m00 * v->x + m10 * v->y + m20,
                    m01 * v->x + m11 * v->y + m21);
            }
        }
        inline ImVec2 Transformed(const ImVec2& v) const
        {
            ImVec2 p = v;
            Transform(&p);
            return p;
        }
    };

    /**
     * Wrapper class around VizDrawList for extra functionality.
     */

    struct VizDrawList {

        ImDrawList& dl;

        std::vector<VizMatrix> trafoStack;

        VizDrawList(ImDrawList& dl) : dl{dl} { }

        std::vector<ImDrawCmd> getCmds () {
            std::vector<ImDrawCmd> cmds;
            for (ImDrawCmd& c : dl.CmdBuffer) { 
                cmds.push_back(c);
            }
            return cmds;
        }

        std::vector<ImDrawVert> getVerts () {
            std::vector<ImDrawVert> verts;
            for (ImDrawVert& v : dl.VtxBuffer) { 
                verts.push_back(v);
            }
            return verts;
        }

        std::vector<ImDrawIdx> getIndices () {
            std::vector<ImDrawIdx> idxs;
            for (ImDrawIdx& i : dl.IdxBuffer) { 
                idxs.push_back(i);
            }
            return idxs;
        }

        std::vector<double> getClipRect() {
            ImVec4& v = dl._ClipRectStack.back();
            return std::vector<double>({v.x, v.y, v.z, v.w});
        }

        void pushTransform(VizMatrix& mat) {

            if (trafoStack.size() > 0) {
                VizMatrix& lastTf = trafoStack.back();
                mat = VizMatrix::Combine(mat, lastTf);
            }

            trafoStack.push_back(mat);
        }

        void pushTransform(array_like<double> trans,
                           double rot,
                           array_like<double> scale) {

            assert_shape(trans, {{2}});
            assert_shape(scale, {{2}});

            VizMatrix mat = VizMatrix::Combine(VizMatrix::Rotation(rot),
                                               VizMatrix::Scaling(scale.at(0), scale.at(1)));
            mat.m20 = trans.at(0);
            mat.m21 = trans.at(1);

            pushTransform(mat);
        }

        void pushPlotTransform() {

            ImPlotPoint a = ImPlot::PrecisePlotToPixels(0.0, 0.0);
            ImPlotPoint b = ImPlot::PrecisePlotToPixels(10.0e7, 10.0e7);

            double scaleX = std::abs((b.x - a.x) / 10.0e7);
            double scaleY = std::abs((b.y - a.y) / 10.0e7);

            VizMatrix mat = VizMatrix::Scaling(scaleX, -scaleY);
            mat.m20 = a.x;
            mat.m21 = a.y;

            pushTransform(mat);
        }

        void pushWindowTransform() {

            ImVec2 pos = ImGui::GetWindowPos();

            VizMatrix mat;
            mat.m20 = pos.x;
            mat.m21 = pos.y;

            pushTransform(mat);
        }

        void popTransform(int count = 1) { 

            for (int i = 0; i < count; ++i) {
                IM_ASSERT(trafoStack.size() > 0);
                trafoStack.pop_back();
            }
        }

        void applyTransform(size_t startIndex) {

            if (trafoStack.size() == 0) {
                return;
            }

            const VizMatrix& m = trafoStack.back();

            if (startIndex < dl._VtxCurrentIdx)
            {
                ImDrawVert* const vertexBegin = dl.VtxBuffer.Data + startIndex;
                ImDrawVert* const vertexEnd   = dl.VtxBuffer.Data + dl._VtxCurrentIdx;

                for (ImDrawVert* vertex = vertexBegin; vertex != vertexEnd; ++vertex)
                {
                    const float x = vertex->pos.x;
                    const float y = vertex->pos.y;

                    vertex->pos.x = m.m00 * x + m.m10 * y + m.m20;
                    vertex->pos.y = m.m01 * x + m.m11 * y + m.m21;
                }
            }
        }

        void pushTransformedClipRect() {

            ImVec2 clMin = dl.GetClipRectMin();
            ImVec2 clMax = dl.GetClipRectMax();

            if (trafoStack.size() == 0) {
                dl.PushClipRect(clMin, clMax);
                return;
            }

            const VizMatrix& m = trafoStack.back();

            const float xMax = clMax.x;
            const float yMax = clMax.y;
            const float xMin = clMin.x;
            const float yMin = clMin.y;

            clMax.x = m.m00 * xMax + m.m10 * yMax - m.m20;
            clMax.y = m.m01 * xMax + m.m11 * yMax - m.m21;

            clMin.x = m.m00 * xMin + m.m10 * yMin - m.m20;
            clMin.y = m.m01 * xMin + m.m11 * yMin - m.m21;

            dl.PushClipRect(clMin, clMax);
        }

        void addVertices(array_like<double>& vertices, py::handle& color) {

            assert_shape(vertices, {{-1, 2}});
            size_t count = vertices.shape(0);

            ImU32 col = ImGui::GetColorU32(interpretColor(color));

            ImVec2 uv(dl._Data->TexUvWhitePixel);
            ImDrawIdx idx = (ImDrawIdx)dl._VtxCurrentIdx;

            dl.PrimReserve(count, count);

            for (size_t i = 0; i < count; ++i) {
                dl._IdxWritePtr[i] = (ImDrawIdx)(idx+i);
                dl._VtxWritePtr[i].pos.x = vertices.at(i, 0);
                dl._VtxWritePtr[i].pos.y = vertices.at(i, 1);
                dl._VtxWritePtr[i].uv = uv;
                dl._VtxWritePtr[i].col = col;
            }

            dl._VtxWritePtr += count;
            dl._VtxCurrentIdx += count;
            dl._IdxWritePtr += count;

            applyTransform(idx);
        }

        void addLine(const ImVec2& p0, const ImVec2& p1, py::handle& color, float width) {

            ImU32 col = ImGui::GetColorU32(interpretColor(color));
            ImVec2 dir = p1 - p0;
            dir /= std::sqrt(dir.x*dir.x + dir.y*dir.y);
            ImVec2 ortho(-dir.y, dir.x);

            float half_width = width * 0.5;
            
            const ImVec2& a = p0 - dir*half_width - ortho*half_width;
            const ImVec2& b = p0 - dir*half_width + ortho*half_width;
            const ImVec2& c = p1 + dir*half_width + ortho*half_width;
            const ImVec2& d = p1 + dir*half_width - ortho*half_width;

            ImVec2 uv(dl._Data->TexUvWhitePixel);
            ImDrawIdx idx = (ImDrawIdx)dl._VtxCurrentIdx;

            dl.PrimReserve(6, 4);

            dl._IdxWritePtr[0] = idx;
            dl._IdxWritePtr[1] = (ImDrawIdx)(idx+1);
            dl._IdxWritePtr[2] = (ImDrawIdx)(idx+2);
            dl._IdxWritePtr[3] = idx;
            dl._IdxWritePtr[4] = (ImDrawIdx)(idx+2);
            dl._IdxWritePtr[5] = (ImDrawIdx)(idx+3);

            dl._VtxWritePtr[0].pos = a;
            dl._VtxWritePtr[0].uv = uv;
            dl._VtxWritePtr[0].col = col;
            dl._VtxWritePtr[1].pos = b;
            dl._VtxWritePtr[1].uv = uv;
            dl._VtxWritePtr[1].col = col;
            dl._VtxWritePtr[2].pos = c;
            dl._VtxWritePtr[2].uv = uv;
            dl._VtxWritePtr[2].col = col;

            dl._VtxWritePtr[3].pos = d;
            dl._VtxWritePtr[3].uv = uv;
            dl._VtxWritePtr[3].col = col;

            dl._VtxWritePtr += 4;
            dl._VtxCurrentIdx += 4;
            dl._IdxWritePtr += 6;

            applyTransform(idx);
        }

        void addRect(const ImVec2& p_min,
                     const ImVec2& p_max,
                     py::handle& fillColor,
                     py::handle& lineColor,
                     float lineWidth) {

            ImVec4 fillCol = interpretColor(fillColor);

            if (fillCol.w != -1 && lineWidth > 0.0) {

                ImU32 col = ImGui::GetColorU32(fillCol);
                
                const ImVec2& a = p_min;
                const ImVec2& c = p_max;

                ImVec2 b(c.x, a.y), d(a.x, c.y), uv(dl._Data->TexUvWhitePixel);
                ImDrawIdx idx = (ImDrawIdx)dl._VtxCurrentIdx;

                dl.PrimReserve(6, 4);

                dl._IdxWritePtr[0] = idx;
                dl._IdxWritePtr[1] = (ImDrawIdx)(idx+1);
                dl._IdxWritePtr[2] = (ImDrawIdx)(idx+2);
                dl._IdxWritePtr[3] = idx;
                dl._IdxWritePtr[4] = (ImDrawIdx)(idx+2);
                dl._IdxWritePtr[5] = (ImDrawIdx)(idx+3);

                dl._VtxWritePtr[0].pos = a;
                dl._VtxWritePtr[0].uv = uv;
                dl._VtxWritePtr[0].col = col;
                dl._VtxWritePtr[1].pos = b;
                dl._VtxWritePtr[1].uv = uv;
                dl._VtxWritePtr[1].col = col;
                dl._VtxWritePtr[2].pos = c;
                dl._VtxWritePtr[2].uv = uv;
                dl._VtxWritePtr[2].col = col;

                dl._VtxWritePtr[3].pos = d;
                dl._VtxWritePtr[3].uv = uv;
                dl._VtxWritePtr[3].col = col;

                dl._VtxWritePtr += 4;
                dl._VtxCurrentIdx += 4;
                dl._IdxWritePtr += 6;

                applyTransform(idx);
            }

            ImVec4 lineCol = interpretColor(lineColor);

            if (lineCol.w != -1) {

                ImU32 col = ImGui::GetColorU32(lineCol);

                ImVec2 offset(lineWidth*0.5, lineWidth*0.5);
                
                const ImVec2 oa = p_min - offset;
                const ImVec2 oc = p_max + offset;
                const ImVec2 ob(oc.x, oa.y);
                const ImVec2 od(oa.x, oc.y);

                const ImVec2 ia = p_min + offset;
                const ImVec2 ic = p_max - offset;
                const ImVec2 ib(ic.x, ia.y);
                const ImVec2 id(ia.x, ic.y);

                ImVec2 uv(dl._Data->TexUvWhitePixel);
                ImDrawIdx idx = (ImDrawIdx)dl._VtxCurrentIdx;

                dl.PrimReserve(24, 8);

                dl._IdxWritePtr[0] = idx;
                dl._IdxWritePtr[1] = (ImDrawIdx)(idx+4);
                dl._IdxWritePtr[2] = (ImDrawIdx)(idx+1);
                dl._IdxWritePtr[3] = (ImDrawIdx)(idx+1);
                dl._IdxWritePtr[4] = (ImDrawIdx)(idx+4);
                dl._IdxWritePtr[5] = (ImDrawIdx)(idx+5);
                dl._IdxWritePtr[6] = (ImDrawIdx)(idx+1);
                dl._IdxWritePtr[7] = (ImDrawIdx)(idx+5);
                dl._IdxWritePtr[8] = (ImDrawIdx)(idx+2);
                dl._IdxWritePtr[9] = (ImDrawIdx)(idx+5);
                dl._IdxWritePtr[10] = (ImDrawIdx)(idx+6);
                dl._IdxWritePtr[11] = (ImDrawIdx)(idx+2);
                dl._IdxWritePtr[12] = (ImDrawIdx)(idx+6);
                dl._IdxWritePtr[13] = (ImDrawIdx)(idx+7);
                dl._IdxWritePtr[14] = (ImDrawIdx)(idx+2);
                dl._IdxWritePtr[15] = (ImDrawIdx)(idx+7);
                dl._IdxWritePtr[16] = (ImDrawIdx)(idx+3);
                dl._IdxWritePtr[17] = (ImDrawIdx)(idx+2);
                dl._IdxWritePtr[18] = (ImDrawIdx)(idx+4);
                dl._IdxWritePtr[19] = (ImDrawIdx)(idx+3);
                dl._IdxWritePtr[20] = (ImDrawIdx)(idx+7);
                dl._IdxWritePtr[21] = idx;
                dl._IdxWritePtr[22] = (ImDrawIdx)(idx+3);
                dl._IdxWritePtr[23] = (ImDrawIdx)(idx+4);

                dl._VtxWritePtr[0].pos = oa;
                dl._VtxWritePtr[0].uv = uv;
                dl._VtxWritePtr[0].col = col;
                dl._VtxWritePtr[1].pos = ob;
                dl._VtxWritePtr[1].uv = uv;
                dl._VtxWritePtr[1].col = col;
                dl._VtxWritePtr[2].pos = oc;
                dl._VtxWritePtr[2].uv = uv;
                dl._VtxWritePtr[2].col = col;
                dl._VtxWritePtr[3].pos = od;
                dl._VtxWritePtr[3].uv = uv;
                dl._VtxWritePtr[3].col = col;
                dl._VtxWritePtr[4].pos = ia;
                dl._VtxWritePtr[4].uv = uv;
                dl._VtxWritePtr[4].col = col;
                dl._VtxWritePtr[5].pos = ib;
                dl._VtxWritePtr[5].uv = uv;
                dl._VtxWritePtr[5].col = col;
                dl._VtxWritePtr[6].pos = ic;
                dl._VtxWritePtr[6].uv = uv;
                dl._VtxWritePtr[6].col = col;
                dl._VtxWritePtr[7].pos = id;
                dl._VtxWritePtr[7].uv = uv;
                dl._VtxWritePtr[7].col = col;

                dl._VtxWritePtr += 8;
                dl._VtxCurrentIdx += 8;
                dl._IdxWritePtr += 24;

                applyTransform(idx);
            }
        }

        void addImage(std::string label,
                      py::array& image,
                      ImVec2 pMin,
                      ImVec2 pMax,
                      ImVec2 uvMin,
                      ImVec2 uvMax,
                      array_like<double> color) {

            ImU32 c = IM_COL32_WHITE;
            if (color.shape(0) != 0) {
                c = ImGui::GetColorU32(interpretColor(color));
            }

            ImageInfo info = interpretImage(image);
            GLuint textureId = uploadImage(label, info, image);

            unsigned int startIndex = dl._VtxCurrentIdx;
            dl.AddImage((void*)(intptr_t)textureId,
                        pMin,
                        pMax,
                        uvMin,
                        uvMax,
                        c);
            applyTransform(startIndex);
        }

        void addBaseNgon(const ImVec2& c,
                         float a,
                         float b,
                         py::handle& fillColor,
                         py::handle& lineColor,
                         float lineWidth,
                         int numSegments) {

            std::vector<ImVec2> dirs(numSegments);

            for (int i = 0; i < numSegments; ++i) {
                double angle = M_PI * 2.0 * (double)i / (double)numSegments;
                dirs[i].x = std::cos(angle);
                dirs[i].y = std::sin(angle);
            }

            ImVec2 uv(dl._Data->TexUvWhitePixel);

            ImVec4 fillCol = interpretColor(fillColor);

            if (fillCol.w != -1) {

                ImDrawIdx idx = (ImDrawIdx)dl._VtxCurrentIdx;
                ImU32 col = ImGui::GetColorU32(fillCol);

                dl.PrimReserve(3*numSegments, 1+numSegments);

                // center vertex
                dl._VtxWritePtr[0].pos = c;
                dl._VtxWritePtr[0].uv = uv;
                dl._VtxWritePtr[0].col = col;

                // first outer vertex
                dl._VtxWritePtr[1].pos.x = c.x + a * dirs[0].x;
                dl._VtxWritePtr[1].pos.y = c.y + b * dirs[0].y;
                dl._VtxWritePtr[1].uv = uv;
                dl._VtxWritePtr[1].col = col;

                for (int i = 0; i < numSegments-1; ++i) {
                    dl._IdxWritePtr[i*3] = idx;
                    dl._IdxWritePtr[i*3+1] = (ImDrawIdx)(idx + i + 1);
                    dl._IdxWritePtr[i*3+2] = (ImDrawIdx)(idx + i + 2);

                    dl._VtxWritePtr[i+2].pos.x = c.x + a * dirs[i+1].x;
                    dl._VtxWritePtr[i+2].pos.y = c.y + b * dirs[i+1].y;
                    dl._VtxWritePtr[i+2].uv = uv;
                    dl._VtxWritePtr[i+2].col = col;
                }

                // final triangle
                dl._IdxWritePtr[(numSegments-1)*3] = idx;
                dl._IdxWritePtr[(numSegments-1)*3+1] = (ImDrawIdx)(idx + numSegments);
                dl._IdxWritePtr[(numSegments-1)*3+2] = (ImDrawIdx)(idx + 1);

                dl._VtxWritePtr += numSegments+1;
                dl._VtxCurrentIdx += numSegments+1;
                dl._IdxWritePtr += 3*numSegments;

                applyTransform(idx);
            }

            ImVec4 lineCol = interpretColor(lineColor);

            if (lineCol.w != -1 && lineWidth > 0) {

                ImDrawIdx idx = (ImDrawIdx)dl._VtxCurrentIdx;
                ImU32 col = ImGui::GetColorU32(lineCol);

                dl.PrimReserve(3*2*numSegments, 2*numSegments);

                double ai = a - lineWidth*0.5;
                double ao = a + lineWidth*0.5;
                double bi = b - lineWidth*0.5;
                double bo = b + lineWidth*0.5;

                // first inner vertex
                dl._VtxWritePtr[0].pos.x = c.x + ai * dirs[0].x;
                dl._VtxWritePtr[0].pos.y = c.y + bi * dirs[0].y;
                dl._VtxWritePtr[0].uv = uv;
                dl._VtxWritePtr[0].col = col;

                // first outer vertex
                dl._VtxWritePtr[1].pos.x = c.x + ao * dirs[0].x;
                dl._VtxWritePtr[1].pos.y = c.y + bo * dirs[0].y;
                dl._VtxWritePtr[1].uv = uv;
                dl._VtxWritePtr[1].col = col;

                for (int i = 0; i < numSegments-1; ++i) {
                    dl._IdxWritePtr[i*6] = (ImDrawIdx)(idx + i*2);
                    dl._IdxWritePtr[i*6+1] = (ImDrawIdx)(idx + i*2 + 1);
                    dl._IdxWritePtr[i*6+2] = (ImDrawIdx)(idx + i*2 + 3);
                    dl._IdxWritePtr[i*6+3] = (ImDrawIdx)(idx + i*2);
                    dl._IdxWritePtr[i*6+4] = (ImDrawIdx)(idx + i*2 + 3);
                    dl._IdxWritePtr[i*6+5] = (ImDrawIdx)(idx + i*2 + 2);

                    dl._VtxWritePtr[(i+1)*2].pos.x = c.x + ai * dirs[i+1].x;
                    dl._VtxWritePtr[(i+1)*2].pos.y = c.y + bi * dirs[i+1].y;
                    dl._VtxWritePtr[(i+1)*2].uv = uv;
                    dl._VtxWritePtr[(i+1)*2].col = col;

                    dl._VtxWritePtr[(i+1)*2+1].pos.x = c.x + ao * dirs[i+1].x;
                    dl._VtxWritePtr[(i+1)*2+1].pos.y = c.y + bo * dirs[i+1].y;
                    dl._VtxWritePtr[(i+1)*2+1].uv = uv;
                    dl._VtxWritePtr[(i+1)*2+1].col = col;
                }

                // final segment
                dl._IdxWritePtr[(numSegments-1)*6] = (ImDrawIdx)(idx + (numSegments-1)*2);
                dl._IdxWritePtr[(numSegments-1)*6+1] = (ImDrawIdx)(idx + (numSegments-1)*2 + 1);
                dl._IdxWritePtr[(numSegments-1)*6+2] = (ImDrawIdx)(idx + 1);
                dl._IdxWritePtr[(numSegments-1)*6+3] = (ImDrawIdx)(idx + (numSegments-1)*2);
                dl._IdxWritePtr[(numSegments-1)*6+4] = (ImDrawIdx)(idx + 1);
                dl._IdxWritePtr[(numSegments-1)*6+5] = (ImDrawIdx)(idx);

                dl._VtxWritePtr += 2*numSegments;
                dl._VtxCurrentIdx += 2*numSegments;
                dl._IdxWritePtr += 3*2*numSegments;

                applyTransform(idx);
            }
        }

        void addText(ImVec2 position,
                     std::string text,
                     array_like<double> color) {

            unsigned int startIndex = dl._VtxCurrentIdx;

            double x = trafoStack.back().m20;
            double y = trafoStack.back().m21;

            dl.AddText(position,
                       ImGui::GetColorU32(interpretColor(color)),
                       text.c_str());

            //dl.AddText(NULL, 0.0f, position, col, text_begin, text_end);

            applyTransform(startIndex);
        }
    };

    m.def("disable_aa", [&]() {
        ImGui::GetCurrentWindow()->DrawList->Flags = ImDrawListFlags_None;
    });

    m.def("get_window_drawlist", [&]() {
        return VizDrawList(*ImGui::GetCurrentWindow()->DrawList);
    });

    m.def("get_plot_drawlist", [&]() {
        return VizDrawList(*ImPlot::GetPlotDrawList());
    });

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

    m.def("get_font_atlas", [](){
        ImGuiIO& io = ImGui::GetIO();
        return io.Fonts;
    }, py::return_value_policy::reference);

    py::class_<ImFontAtlas>(m, "FontAtlas")
        .def("get_texture_id", [&](ImFontAtlas& a) {
            return (size_t)a.TexID;
        })
        .def("get_fonts", [&](ImFontAtlas& a) {
            std::vector<ImFont*> fonts;
            for (ImFont* f : a.Fonts) { 
                fonts.push_back(f);
            }
            return fonts;
        }, py::return_value_policy::reference);

    py::class_<ImFont>(m, "Font")
        .def_readonly("font_size", &ImFont::FontSize)
        .def("get_index_lookup", [&](ImFont& f) {
            std::vector<int> indexLookup;
            for (ImWchar& c : f.IndexLookup) {
                indexLookup.push_back(c);
            }
            return indexLookup;
        })
        .def("get_glyphs", [&](ImFont& f) {
            std::vector<ImFontGlyph> glyphs;
            for (ImFontGlyph& g : f.Glyphs) {
                glyphs.push_back(g);
            }
            return glyphs;
        });

    py::class_<ImFontGlyph>(m, "ImFontGlyph")
        .def_property_readonly("codepoint", [](ImFontGlyph& g) {
                return (unsigned int)g.Codepoint;
        })
        .def_readonly("x0", &ImFontGlyph::X0)
        .def_readonly("y0", &ImFontGlyph::Y0)
        .def_readonly("x1", &ImFontGlyph::X1)
        .def_readonly("y1", &ImFontGlyph::Y1)
        .def_readonly("u0", &ImFontGlyph::U0)
        .def_readonly("v0", &ImFontGlyph::V0)
        .def_readonly("u1", &ImFontGlyph::U1)
        .def_readonly("v1", &ImFontGlyph::V1)
        .def_readonly("advance_x", &ImFontGlyph::AdvanceX);

    py::class_<ImDrawCmd>(m, "DrawCmd")
        .def_readwrite("clip_rect", &ImDrawCmd::ClipRect)
        .def_property_readonly("texture_id", [](ImDrawCmd& cmd) {
            return (size_t)cmd.TextureId;
        })
        .def_readwrite("vtx_offset", &ImDrawCmd::VtxOffset)
        .def_readwrite("idx_offset", &ImDrawCmd::IdxOffset)
        .def_readwrite("elem_count", &ImDrawCmd::ElemCount);

    py::class_<ImDrawVert>(m, "DrawVert")
        .def_readwrite("pos", &ImDrawVert::pos)
        .def_readwrite("uv", &ImDrawVert::uv)
        .def_readwrite("col", &ImDrawVert::col);

    py::class_<VizDrawList>(m, "VizDrawList")
        .def("get_cmds", &VizDrawList::getCmds)
        .def("get_verts", &VizDrawList::getVerts)
        .def("get_indices", &VizDrawList::getIndices)
        .def("get_clip_rect", &VizDrawList::getClipRect)
        .def("push_plot_transform", &VizDrawList::pushPlotTransform)
        .def("push_transformed_clip_rect", &VizDrawList::pushTransformedClipRect)
        .def("push_window_transform", &VizDrawList::pushWindowTransform)
        .def("push_transform", [&](VizDrawList& vdl,
                                   array_like<double> trans,
                                   double rot,
                                   array_like<double> scale) {
            vdl.pushTransform(trans, rot, scale); 
        },
        py::arg("trans") = ImVec2(0.0, 0.0),
        py::arg("rot") = 0.0,
        py::arg("scale") = ImVec2(1.0, 1.0))
        .def("pop_transform", &VizDrawList::popTransform, py::arg("count") = 1)
        .def("add_vertices", &VizDrawList::addVertices,
            py::arg("vertices"),
            py::arg("color") = py::array())
        .def("add_line", &VizDrawList::addLine,
            py::arg("p0"),
            py::arg("p1"),
            py::arg("color") = py::array(),
            py::arg("width") = 1.0)
        .def("add_rect", &VizDrawList::addRect,
            py::arg("p_min"),
            py::arg("p_max"),
            py::arg("fill_color") = py::array(),
            py::arg("line_color") = py::array(),
            py::arg("line_width") = 1.0)
        .def("add_image", &VizDrawList::addImage,
            py::arg("label"),
            py::arg("image"),
            py::arg("p_min"),
            py::arg("p_max"),
            py::arg("uv_min") = ImVec2(0, 0),
            py::arg("uv_max") = ImVec2(1, 1),
            py::arg("color") = py::array())
        .def("add_ellipse", &VizDrawList::addBaseNgon,
            py::arg("center"),
            py::arg("a"),
            py::arg("b"),
            py::arg("fill_color") = py::array(),
            py::arg("line_color") = py::array(),
            py::arg("line_width") = 1.0,
            py::arg("num_segments") = 64)
        .def("add_circle", [](VizDrawList& vdl,
                              const ImVec2& c,
                              float r,
                              py::handle& fillColor,
                              py::handle& lineColor,
                              float lineWidth,
                              int numSegments) {
                vdl.addBaseNgon(c, r, r, fillColor, lineColor, lineWidth, numSegments);
            },
            py::arg("center"),
            py::arg("r"),
            py::arg("fill_color") = py::array(),
            py::arg("line_color") = py::array(),
            py::arg("line_width") = 1.0,
            py::arg("num_segments") = 64)
        .def("add_text", &VizDrawList::addText,
            py::arg("position"),
            py::arg("text"),
            py::arg("color"));
}

void resetDragDrop() {

    if (dragDropClearCounter > 0) {
        dragDropClearCounter -= 1;
    } else {
        dragDropRef = py::none();
    }
}
