#include "bindings_implot.hpp"
#include "binding_helpers.hpp"
#include "imviz.hpp"

#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <functional>

#include <pybind11/pytypes.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_styles.h"
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
    #pragma region Flags and defines
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

	py::enum_<ImGuiSelectableFlags_>(m, "SelectableFlags", py::arithmetic())
		.value("NONE", ImGuiSelectableFlags_None)
		.value("DONT_CLOSE_POPUPS", ImGuiSelectableFlags_DontClosePopups)
		.value("SPAN_ALL_COLUMNS", ImGuiSelectableFlags_SpanAllColumns)
		.value("ALLOW_DOUBLE_CLICK", ImGuiSelectableFlags_AllowDoubleClick)
		.value("DISABLED", ImGuiSelectableFlags_Disabled)
		.value("ALLOW_OVERLAP", ImGuiSelectableFlags_AllowOverlap);

	py::enum_<ImGuiSliderFlags_>(m, "SliderFlags", py::arithmetic())
		.value("NONE", ImGuiSliderFlags_None)
		.value("ALWAYS_CLAMP", ImGuiSliderFlags_AlwaysClamp)
		.value("LOGARITHMIC", ImGuiSliderFlags_Logarithmic)
		.value("NO_ROUND_TO_FORMAT", ImGuiSliderFlags_NoRoundToFormat)
		.value("NO_INPUT", ImGuiSliderFlags_NoInput);
	
	py::enum_<ImGuiInputTextFlags_>(m, "InputTextFlags", py::arithmetic())
		.value("None", ImGuiInputTextFlags_None)
		.value("CHARS_DECIMAL", ImGuiInputTextFlags_CharsDecimal)
		.value("CHARS_HEXADECIMAL", ImGuiInputTextFlags_CharsHexadecimal)
		.value("CHARS_UPPERCASE", ImGuiInputTextFlags_CharsUppercase)
		.value("CHARS_NO_BLANK", ImGuiInputTextFlags_CharsNoBlank)
		.value("AUTO_SELECT_ALL", ImGuiInputTextFlags_AutoSelectAll)
		.value("ENTER_RETURNS_TRUE", ImGuiInputTextFlags_EnterReturnsTrue)
		.value("CALLBACK_COMPLETION", ImGuiInputTextFlags_CallbackCompletion)
		.value("CALLBACK_HISTORY", ImGuiInputTextFlags_CallbackHistory)
		.value("CALLBACK_ALWAYS", ImGuiInputTextFlags_CallbackAlways)
		.value("CALLBACK_CHAR_FILTER", ImGuiInputTextFlags_CallbackCharFilter)
		.value("ALLOW_TAB_INPUT", ImGuiInputTextFlags_AllowTabInput)
		.value("CTRL_ENTER_FOR_NEW_LINE", ImGuiInputTextFlags_CtrlEnterForNewLine)
		.value("NO_HORIZONTAL_SCROLL", ImGuiInputTextFlags_NoHorizontalScroll)
		.value("ALWAYS_OVERWRITE", ImGuiInputTextFlags_AlwaysOverwrite)
		.value("READ_ONLY", ImGuiInputTextFlags_ReadOnly)
		.value("PASSWORD", ImGuiInputTextFlags_Password)
		.value("NO_UNDO_REDO", ImGuiInputTextFlags_NoUndoRedo)
		.value("CHARS_SCIENTIFIC", ImGuiInputTextFlags_CharsScientific)
		.value("CALLBACK_RESIZE", ImGuiInputTextFlags_CallbackResize)
		.value("CALLBACK_EDIT", ImGuiInputTextFlags_CallbackEdit)
		.value("ESCAPE_CLEARS_ALL", ImGuiInputTextFlags_EscapeClearsAll);
	
	py::enum_<ImGuiInputFlags_>(m, "InputFlags", py::arithmetic())
		.value("NONE", ImGuiInputFlags_None)
		.value("REPEAT", ImGuiInputFlags_Repeat)
		.value("REPEAT_RATE_DEFAULT", ImGuiInputFlags_RepeatRateDefault)
		.value("REPEAT_RATE_NAV_MOVE", ImGuiInputFlags_RepeatRateNavMove)
		.value("REPEAT_RATE_NAV_TWEAK", ImGuiInputFlags_RepeatRateNavTweak)
		.value("REPEAT_UNTIL_RELEASE", ImGuiInputFlags_RepeatUntilRelease)
		.value("REPEAT_UNTIL_KEY_MODS_CHANGE", ImGuiInputFlags_RepeatUntilKeyModsChange)
		.value("REPEAT_UNTIL_KEY_MODS_CHANGE_FROM_NONE", ImGuiInputFlags_RepeatUntilKeyModsChangeFromNone)
		.value("REPEAT_UNTIL_OTHER_KEY_PRESS", ImGuiInputFlags_RepeatUntilOtherKeyPress)
		.value("COND_HOVERED", ImGuiInputFlags_CondHovered)
		.value("COND_ACTIVE", ImGuiInputFlags_CondActive)
		.value("LOCK_THIS_FRAME", ImGuiInputFlags_LockThisFrame)
		.value("LOCK_UNTIL_RELEASE", ImGuiInputFlags_LockUntilRelease)
		.value("ROUTE_FOCUSED", ImGuiInputFlags_RouteFocused)
		.value("ROUTE_GLOBAL_LOW", ImGuiInputFlags_RouteGlobalLow)
		.value("ROUTE_GLOBAL", ImGuiInputFlags_RouteGlobal)
		.value("ROUTE_GLOBAL_HIGH", ImGuiInputFlags_RouteGlobalHigh)
		.value("ROUTE_ALWAYS", ImGuiInputFlags_RouteAlways)
		.value("ROUTE_UNLESS_BG_FOCUSED", ImGuiInputFlags_RouteUnlessBgFocused);

	py::enum_<ImGuiWindowFlags_>(m, "WindowFlags", py::arithmetic())
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
        .value("ANGLED_HEADER", ImGuiTableColumnFlags_AngledHeader)
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

    py::enum_<ImGuiDockNodeFlags_>(m, "DockNodeFlags", py::arithmetic())
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

    #pragma endregion

    #pragma region Widgets

    m.def("show_imgui_demo", ImGui::ShowDemoWindow);
    m.def("show_implot_demo", ImPlot::ShowDemoWindow);

    m.def("begin_window", [&](std::string label,
                       bool opened,
                       bool show_close_btn,
					   ImGuiWindowFlags flags) {
        viz.currentWindowOpen = opened;
		if (!opened) return opened;

		bool show;
		if (show_close_btn)
			show = ImGui::Begin(label.c_str(), &viz.currentWindowOpen, flags);
		else
			show = ImGui::Begin(label.c_str(), nullptr, flags);

        return show;
    },
    py::arg("label"),
    py::arg("opened") = true,
    py::arg("show_close_btn") = false,
    py::arg("flags") = ImGuiWindowFlags_None);

    m.def("end_window", ImGui::End);

    m.def("begin_child", [](std::string label, ImVec2 size, bool border, ImGuiWindowFlags flags){

        return ImGui::BeginChild(label.c_str(), size, border, flags);
    },
    py::arg("label"),
    py::arg("size") = ImVec2(0, 0),
    py::arg("border") = false,
    py::arg("flags") = ImGuiWindowFlags_None);

    m.def("end_child", ImGui::EndChild);

    m.def("begin_popup_context_item", [&](std::string label) {

        return ImGui::BeginPopupContextItem(
                label.empty() ? 0 : label.c_str());
    },
    py::arg("label") = "");

    m.def("begin_popup", [&](std::string label, ImGuiWindowFlags flags) {

        return ImGui::BeginPopup(label.c_str(), flags);
    },
    py::arg("label"),
	py::arg("flags") = ImGuiWindowFlags_None);

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

	m.def("push_button_repeat", ImGui::PushButtonRepeat,
	py::arg("repeat") = true);
	m.def("pop_button_repeat", ImGui::PopButtonRepeat);

	m.def("button", ImGui::Button,
    py::arg("label"),
    py::arg("size") = ImVec2(0, 0));

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

	m.def("calc_text_size", [&](std::string str) {
		return ImGui::CalcTextSize(str.c_str());
	},
    py::arg("str"));

    m.def("text", [&](py::handle obj, py::handle color) {
		
		ImVec4 c = interpretColor(color);
		
        std::string str = py::str(obj);
		
        if (c.w >= 0) {
			ImGui::PushStyleColor(ImGuiCol_Text, c);
			ImGui::TextUnformatted(str.c_str());
			ImGui::PopStyleColor();
        } else {
			ImGui::TextUnformatted(str.c_str());
        }
    },
    py::arg("str"),
    py::arg("color") = py::array());
	m.def("text_wrapped", [&](py::handle obj) {
        std::string str = py::str(obj);
		ImGui::TextWrapped("%s", str.c_str());
    },
    py::arg("str"));

	m.def("progress_bar", [&](float fraction, ImVec2 size = ImVec2(0, 0), std::string overlay = "") {
		ImGui::ProgressBar(fraction, size, overlay.c_str());
	},
	py::arg("fraction"),
	py::arg("size") = ImVec2(0, 0),
	py::arg("overlay") = "");

    m.def("input", [&](std::string label, std::string& obj, std::string hint, ImGuiInputTextFlags flags, std::function<void (std::string)>* callback) {
		bool mod;
		if (hint.empty()) {
			mod = ImGui::InputText(label.c_str(), &obj, flags, [](ImGuiInputTextCallbackData* data) {
				auto callback = static_cast<std::function<void (std::string)>*>(data->UserData);
				if (callback) {
					(*callback)(data->Buf);
				}
				return 0;
			}, callback);
		} else {
			mod = ImGui::InputTextWithHint(label.c_str(), hint.c_str(), &obj, flags, [](ImGuiInputTextCallbackData* data) {
				auto callback = static_cast<std::function<void (std::string)>*>(data->UserData);
				if (callback) {
					(*callback)(data->Buf);
				}
				return 0;
			}, callback);
		}
		viz.setMod(mod);
		return obj;
	},
	py::arg("label"),
	py::arg("value"),
	py::arg("hint") = "",
	py::arg("flags") = 0,
	py::arg("callback") = nullptr);
	m.def("input_multiline", [&](std::string label, std::string& obj, ImVec2 size, ImGuiInputTextFlags flags, std::function<void (std::string)>* callback) {
		bool mod = ImGui::InputTextMultiline(
			label.c_str(), &obj, size, flags, [](ImGuiInputTextCallbackData* data) {
				auto callback = static_cast<std::function<void (std::string)>*>(data->UserData);
				if (callback) {
					(*callback)(data->Buf);
				}
				return 0;
			}, callback);
		viz.setMod(mod);
		return obj;
	},
	py::arg("label"),
	py::arg("value"),
	py::arg("size") = ImVec2(0, 0),
	py::arg("flags") = 0,
	py::arg("callback") = nullptr);
    m.def("input", [&](std::string label, int64_t& obj, int64_t step, int64_t step_fast, std::string format, ImGuiInputTextFlags flags) {
        bool mod = ImGui::InputScalar(
			label.c_str(), ImGuiDataType_S64, &obj, &step, &step_fast, format.c_str(), flags);
        viz.setMod(mod);
        return obj;
    },
    py::arg("label"),
    py::arg("value"),
    py::arg("step") = 1,
    py::arg("step_fast") = 100,
    py::arg("format") = "%i",
    py::arg("flags") = ImGuiInputTextFlags_None);
    m.def("input", [&](std::string label, double& obj, double step, double step_fast, std::string format, ImGuiInputTextFlags flags) {
        bool mod = ImGui::InputScalar(
			label.c_str(), ImGuiDataType_Double, &obj, &step, &step_fast, format.c_str(), flags);
        viz.setMod(mod);
        return obj;
    },
    py::arg("label"),
    py::arg("value"),
    py::arg("step") = 1.0,
    py::arg("step_fast") = 100.0,
    py::arg("format") = "%.1f",
    py::arg("flags") = ImGuiInputTextFlags_None);

	m.def("input_int_nd", [&](std::string label, array_like<int64_t>& obj, int64_t step, int64_t step_fast, std::string format, ImGuiInputTextFlags flags) {
		bool mod = ImGui::InputScalarN(
			label.c_str(), ImGuiDataType_S64, obj.mutable_data(), static_cast<int>(obj.size()), &step, &step_fast, format.c_str(), flags);
		viz.setMod(mod);
		return obj;
	},
	py::arg("label"),
	py::arg("value"),
	py::arg("step") = 1,
	py::arg("step_fast") = 100,
	py::arg("format") = "%i",
	py::arg("flags") = ImGuiInputTextFlags_None);
	m.def("input_float_nd", [&](std::string label, array_like<double>& obj, double step, double step_fast, std::string format, ImGuiInputTextFlags flags) {
		bool mod = ImGui::InputScalarN(
			label.c_str(), ImGuiDataType_Double, obj.mutable_data(), static_cast<int>(obj.size()), &step, &step_fast, format.c_str(), flags);
		viz.setMod(mod);
		return obj;
	},
	py::arg("label"),
	py::arg("value"),
	py::arg("step") = 1.0,
	py::arg("step_fast") = 100.0,
	py::arg("format") = "%.1f",
	py::arg("flags") = ImGuiInputTextFlags_None);

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
    py::arg("value"));

	m.def("vslider", [&](std::string label, int64_t& value, ImVec2 size, int64_t min, int64_t max, std::string format, ImGuiSliderFlags flags) {
        bool mod = ImGui::VSliderScalar(
                label.c_str(), size, ImGuiDataType_S64, &value, &min, &max, format.c_str(), flags);
        viz.setMod(mod);
        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
	py::arg("size"),
    py::arg("min") = 0,
    py::arg("max") = 100,
    py::arg("format") = "%i",
    py::arg("flags") = ImGuiSliderFlags_None);
	m.def("vslider", [&](std::string label, double& value, ImVec2 size, double min, double max, std::string format, ImGuiSliderFlags flags) {
        bool mod = ImGui::VSliderScalar(
                label.c_str(), size, ImGuiDataType_Double, &value, &min, &max, format.c_str(), flags);
        viz.setMod(mod);
        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
	py::arg("size"),
    py::arg("min") = 0.0,
    py::arg("max") = 1.0,
    py::arg("format") = "%.1f",
    py::arg("flags") = ImGuiSliderFlags_None);

	m.def("slider", [&](std::string label, int64_t& value, int64_t min, int64_t max, std::string format, ImGuiSliderFlags flags) {
        bool mod = ImGui::SliderScalar(
                label.c_str(), ImGuiDataType_S64, &value, &min, &max, format.c_str(), flags);
        viz.setMod(mod);
        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
    py::arg("min") = 0,
    py::arg("max") = 100,
    py::arg("format") = "%i",
    py::arg("flags") = ImGuiSliderFlags_None);
    m.def("slider", [&](std::string label, double& value, double min, double max, std::string format, ImGuiSliderFlags flags) {
        bool mod = ImGui::SliderScalar(
                label.c_str(), ImGuiDataType_Double, &value, &min, &max, format.c_str(), flags);
        viz.setMod(mod);
        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
    py::arg("min") = 0.0,
    py::arg("max") = 1.0,
    py::arg("format") = "%.1f",
    py::arg("flags") = ImGuiSliderFlags_None);

	m.def("slider_int_nd", [&](std::string label, array_like<int64_t>& values, int64_t min, int64_t max, std::string format, ImGuiSliderFlags flags) {
		bool mod = ImGui::SliderScalarN(
			label.c_str(), ImGuiDataType_S64, values.mutable_data(), static_cast<int>(values.size()), &min, &max, format.c_str(), flags);
		viz.setMod(mod);
		return values;
	},
	py::arg("label"),
	py::arg("values"),
	py::arg("min") = 0,
	py::arg("max") = 100,
	py::arg("format") = "%i",
	py::arg("flags") = ImGuiSliderFlags_None);
	m.def("slider_float_nd", [&](std::string label, array_like<double>& values, double min, double max, std::string format, ImGuiSliderFlags flags) {
		bool mod = ImGui::SliderScalarN(
			label.c_str(), ImGuiDataType_Double, values.mutable_data(), static_cast<int>(values.size()), &min, &max, format.c_str(), flags);
		viz.setMod(mod);
		return values;
	},
	py::arg("label"),
	py::arg("values"),
	py::arg("min") = 0.0,
	py::arg("max") = 1.0,
	py::arg("format") = "%.1f",
	py::arg("flags") = ImGuiSliderFlags_None);

    m.def("drag", [&](std::string label, int64_t& value, float speed, int64_t min, int64_t max, std::string format, ImGuiSliderFlags flags) {

        bool mod = ImGui::DragScalar(
			label.c_str(), ImGuiDataType_S64, &value, speed, &min, &max, format.c_str(), flags);
        viz.setMod(mod);

        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
    py::arg("speed") = 1.0,
    py::arg("min") = 0,
    py::arg("max") = 0,
    py::arg("format") = "%i",
    py::arg("flags") = ImGuiSliderFlags_None);
	m.def("drag", [&](std::string label, double& value, float speed, double min, double max, std::string format, ImGuiSliderFlags flags) {
        
        bool mod = ImGui::DragScalar(label.c_str(),
                ImGuiDataType_Double, &value, speed, &min, &max, format.c_str(), flags);
        viz.setMod(mod);

        return value;
    }, 
    py::arg("label"),
    py::arg("value"),
    py::arg("speed") = 0.1,
    py::arg("min") = 0.0,
    py::arg("max") = 0.0,
    py::arg("format") = "%.1f",
    py::arg("flags") = ImGuiSliderFlags_None);

	m.def("drag_int_nd", [&](std::string title, array_like<int64_t>& values, float speed, int64_t min, int64_t max, std::string format, ImGuiSliderFlags flags) {
		bool mod = ImGui::DragScalarN(
			title.c_str(), ImGuiDataType_S64, values.mutable_data(), static_cast<int>(values.size()),
			speed, &min, &max, format.c_str(), flags);
		viz.setMod(mod);
		return values;
	},
	py::arg("label"),
	py::arg("values"),
	py::arg("speed") = 1.0,
	py::arg("min") = 0,
	py::arg("max") = 0,
	py::arg("format") = "%i",
	py::arg("flags") = ImGuiSliderFlags_None);
	m.def("drag_float_nd", [&](std::string title, array_like<double>& values, float speed, double min, double max, std::string format, ImGuiSliderFlags flags) {
		bool mod = ImGui::DragScalarN(
			title.c_str(), ImGuiDataType_Double, values.mutable_data(), static_cast<int>(values.size()),
			speed, &min, &max, format.c_str(), flags);
		viz.setMod(mod);
		return values;
	},
	py::arg("label"),
	py::arg("values"),
	py::arg("speed") = 0.1,
	py::arg("min") = 0.0,
	py::arg("max") = 0.0,
	py::arg("format") = "%.1f",
	py::arg("flags") = ImGuiSliderFlags_None);
	
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
    m.def("separator_text", ImGui::SeparatorText, py::arg("text"));

    m.def("begin_tooltip", ImGui::BeginTooltip);
    m.def("end_tooltip", ImGui::EndTooltip);
    m.def("set_item_tooltip", [](std::string tooltip) {
		ImGui::SetItemTooltip("%s", tooltip.c_str());
	},
	py::arg("tooltip"));

    m.def("selectable", [&](std::string label, bool selected, ImVec2 size, ImGuiSelectableFlags flags) { 

        bool s = ImGui::Selectable(label.c_str(), selected, flags, size);

        if (selected != s) {
            viz.setMod(true);
        }

        return s;
    },
    py::arg("label"),
    py::arg("selected"),
    py::arg("size") = ImVec2(0, 0),
	py::arg("flags") = ImGuiSelectableFlags_None);

    #pragma endregion

    #pragma region Tables & Columns

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
    m.def("table_angled_headers_row", &ImGui::TableAngledHeadersRow);

    m.def("table_setup_scroll_freeze", &ImGui::TableSetupScrollFreeze,
	py::arg("cols"),
	py::arg("rows"));

    #pragma endregion

    #pragma region Layout functions

    m.def("set_scroll_here_x", ImGui::SetScrollHereX,
	py::arg("align") = 0.0f);
    m.def("set_scroll_here_y", ImGui::SetScrollHereY,
	py::arg("align") = 0.0f);
	m.def("get_scroll_x", ImGui::GetScrollX);
	m.def("get_scroll_y", ImGui::GetScrollY);
	m.def("get_scroll_max_x", ImGui::GetScrollMaxX);
	m.def("get_scroll_max_y", ImGui::GetScrollMaxY);

	m.def("align_text_to_frame_padding", ImGui::AlignTextToFramePadding);

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

    #pragma endregion

    #pragma region Config helper functions

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

    #pragma endregion

    #pragma region Window helper functions

    m.def("set_next_window_pos", ImGui::SetNextWindowPos,
    py::arg("position"),
    py::arg("cond") = ImGuiCond_None,
    py::arg("pivot") = py::array());
	m.def("set_window_pos", [](ImVec2 pos, ImGuiCond cond) {
		ImGui::SetWindowPos(pos, cond);
	},
	py::arg("position"),
	py::arg("cond") = ImGuiCond_None);
	m.def("set_window_pos", [](std::string label, ImVec2 pos, ImGuiCond cond) {
		ImGui::SetWindowPos(label.c_str(), pos, cond);
	},
	py::arg("label"),
	py::arg("position"),
	py::arg("cond") = ImGuiCond_None);

    m.def("set_next_window_size", ImGui::SetNextWindowSize,
    py::arg("size"),
    py::arg("cond") = ImGuiCond_None);
	m.def("set_window_size", [](ImVec2 size, ImGuiCond cond) {
		ImGui::SetWindowSize(size, cond);
	},
	py::arg("size"),
	py::arg("cond") = ImGuiCond_None);
	m.def("set_window_size", [](std::string label, ImVec2 size, ImGuiCond cond) {
		ImGui::SetWindowSize(label.c_str(), size, cond);
	},
	py::arg("label"),
	py::arg("size"),
	py::arg("cond") = ImGuiCond_None);

	m.def("set_next_window_focus", ImGui::SetNextWindowFocus);
	m.def("set_window_focus", []() {
		ImGui::SetWindowFocus();
	});
	m.def("set_window_focus", [](std::string label) {
		ImGui::SetWindowFocus(label.c_str());
	},
	py::arg("label"));

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

    #pragma endregion

    #pragma region Font functions

    m.def("get_global_font_size", [&]() {
        return viz.smallFont->FontSize;
    });
    m.def("set_global_font_size", [&](double baseSize) {
        return viz.fontBaseSize = baseSize;
    });

	m.def("get_window_font_scale", []() {
		return ImGui::GetCurrentWindow()->FontWindowScale;
	});
	m.def("set_window_font_scale", ImGui::SetWindowFontScale, py::arg("scale") = 1.0f);

    #pragma endregion

    #pragma region Item helper functions

    m.def("set_next_item_width", 
            ImGui::SetNextItemWidth,
    py::arg("width"));

	m.def("push_item_width", ImGui::PushItemWidth,
	py::arg("width"));

	m.def("pop_item_width", ImGui::PopItemWidth);

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

    #pragma endregion

    #pragma region ID management functions

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

    #pragma endregion

    #pragma region Styling

	m.def("scale_all_sizes", [](float scale) {
		ImGui::GetStyle().ScaleAllSizes(scale);
	}, py::arg("scale"));

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
	m.def("push_style_var", [](ImGuiStyleVar idx, float val){
        ImGui::PushStyleVar(idx, val);
    },
    py::arg("idx"),
    py::arg("val"));

    m.def("pop_style_var", [](int count) {
        ImGui::PopStyleVar(count);
    }, 
    py::arg("count") = 1);

	m.def("style_colors_dark", [&](){
        ImGui::StyleColorsDark();
        ImPlot::StyleColorsDark();
    });
    m.def("style_colors_light", [&](){
        ImGui::StyleColorsLight();
        ImPlot::StyleColorsLight();
    });
	m.def("style_colors_classic", [&](){
        ImGui::StyleColorsClassic();
        ImPlot::StyleColorsClassic();
    });

	#define HelpMarker(desc)										\
	{																\
		ImGui::TextDisabled("(?)");									\
		if (ImGui::BeginItemTooltip())								\
		{															\
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);	\
			ImGui::TextUnformatted(desc);							\
			ImGui::PopTextWrapPos();								\
			ImGui::EndTooltip();									\
		}															\
	}

	m.def("style_editor", [&]() {
		auto& style = ImGui::GetStyle();

		ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);

		// Save button
		static std::string filePath = "";
		ImGui::InputTextWithHint("##save_path", "INI path", &filePath);
		ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
		if (ImGui::Button("Save") && !filePath.empty())
			ImGui::SaveStylesTo(filePath.c_str());
		ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
		if (ImGui::Button("Load") && !filePath.empty())
		{
			ImGui::LoadStyleFrom(filePath.c_str());
			ImPlot::StyleColorsAuto();
		}

		ImGui::Separator();

		if (ImGui::BeginTabBar("##tabs"))
		{
			if (ImGui::BeginTabItem("Sizes"))
			{
				ImGui::SeparatorText("Main");
				ImGui::SliderFloat2("WindowPadding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("ItemInnerSpacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("TouchExtraPadding", (float*)&style.TouchExtraPadding, 0.0f, 10.0f, "%.0f");
				ImGui::SliderFloat("IndentSpacing", &style.IndentSpacing, 0.0f, 30.0f, "%.0f");
				ImGui::SliderFloat("ScrollbarSize", &style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
				ImGui::SliderFloat("GrabMinSize", &style.GrabMinSize, 1.0f, 20.0f, "%.0f");

				ImGui::SeparatorText("Borders");
				ImGui::SliderFloat("WindowBorderSize", &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("ChildBorderSize", &style.ChildBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("PopupBorderSize", &style.PopupBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("FrameBorderSize", &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("TabBorderSize", &style.TabBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("TabBarBorderSize", &style.TabBarBorderSize, 0.0f, 2.0f, "%.0f");

				ImGui::SeparatorText("Rounding");
				ImGui::SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("ChildRounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("PopupRounding", &style.PopupRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("ScrollbarRounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("GrabRounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("TabRounding", &style.TabRounding, 0.0f, 12.0f, "%.0f");

				ImGui::SeparatorText("Tables");
				ImGui::SliderFloat2("CellPadding", (float*)&style.CellPadding, 0.0f, 20.0f, "%.0f");
				ImGui::SliderAngle("TableAngledHeadersAngle", &style.TableAngledHeadersAngle, -50.0f, +50.0f);
				ImGui::SliderFloat2("TableAngledHeadersTextAlign", (float*)&style.TableAngledHeadersTextAlign, 0.0f, 1.0f, "%.2f");

				ImGui::SeparatorText("Widgets");
				ImGui::SliderFloat2("WindowTitleAlign", (float*)&style.WindowTitleAlign, 0.0f, 1.0f, "%.2f");
				int window_menu_button_position = style.WindowMenuButtonPosition + 1;
				if (ImGui::Combo("WindowMenuButtonPosition", (int*)&window_menu_button_position, "None\0Left\0Right\0"))
					style.WindowMenuButtonPosition = window_menu_button_position - 1;
				ImGui::Combo("ColorButtonPosition", (int*)&style.ColorButtonPosition, "Left\0Right\0");
				ImGui::SliderFloat2("ButtonTextAlign", (float*)&style.ButtonTextAlign, 0.0f, 1.0f, "%.2f");
				ImGui::SameLine();
				HelpMarker("Alignment applies when a button is larger than its text content.");
				ImGui::SliderFloat2("SelectableTextAlign", (float*)&style.SelectableTextAlign, 0.0f, 1.0f, "%.2f");
				ImGui::SameLine(); HelpMarker("Alignment applies when a selectable is larger than its text content.");
				ImGui::SliderFloat("SeparatorTextBorderSize", &style.SeparatorTextBorderSize, 0.0f, 10.0f, "%.0f");
				ImGui::SliderFloat2("SeparatorTextAlign", (float*)&style.SeparatorTextAlign, 0.0f, 1.0f, "%.2f");
				ImGui::SliderFloat2("SeparatorTextPadding", (float*)&style.SeparatorTextPadding, 0.0f, 40.0f, "%.0f");
				ImGui::SliderFloat("LogSliderDeadzone", &style.LogSliderDeadzone, 0.0f, 12.0f, "%.0f");

				ImGui::SeparatorText("Docking");
				ImGui::SliderFloat("DockingSplitterSize", &style.DockingSeparatorSize, 0.0f, 12.0f, "%.0f");

				ImGui::SeparatorText("Misc");
				ImGui::SliderFloat2("DisplaySafeAreaPadding", (float*)&style.DisplaySafeAreaPadding, 0.0f, 30.0f, "%.0f"); ImGui::SameLine(); HelpMarker("Adjust if you cannot see the edges of your screen (e.g. on a TV where scaling has not been configured).");

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Colors"))
			{
				static ImGuiTextFilter filter;
				filter.Draw("Filter colors", ImGui::GetFontSize() * 16);

				static ImGuiColorEditFlags alpha_flags = 0;
				if (ImGui::RadioButton("Opaque", alpha_flags == ImGuiColorEditFlags_None))             { alpha_flags = ImGuiColorEditFlags_None; } ImGui::SameLine();
				if (ImGui::RadioButton("Alpha",  alpha_flags == ImGuiColorEditFlags_AlphaPreview))     { alpha_flags = ImGuiColorEditFlags_AlphaPreview; } ImGui::SameLine();
				if (ImGui::RadioButton("Both",   alpha_flags == ImGuiColorEditFlags_AlphaPreviewHalf)) { alpha_flags = ImGuiColorEditFlags_AlphaPreviewHalf; } ImGui::SameLine();
				HelpMarker(
					"In the color list:\n"
					"Left-click on color square to open color picker,\n"
					"Right-click to open edit options menu.");

				ImGui::PushItemWidth(ImGui::GetFontSize() * -12);
				for (int i = 0; i < ImGuiCol_COUNT; i++)
				{
					const char* name = ImGui::GetStyleColorName(i);
					if (!filter.PassFilter(name))
						continue;
					ImGui::PushID(i);
				
					#ifndef IMGUI_DISABLE_DEBUG_TOOLS
					if (ImGui::Button("?"))
						ImGui::DebugFlashStyleColor((ImGuiCol)i);
					ImGui::SetItemTooltip("Flash given color to identify places where it is used.");
					ImGui::SameLine();
					#endif
				
					ImGui::ColorEdit4("##color", (float*)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | alpha_flags);
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
					ImGui::TextUnformatted(name);
					ImGui::PopID();
				}
				ImGui::PopItemWidth();

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Fonts"))
			{
				ImGuiIO& io = ImGui::GetIO();
				ImFontAtlas* atlas = io.Fonts;
				HelpMarker("Read FAQ and docs/FONTS.md for details on font loading.");
				ImGui::ShowFontAtlas(atlas);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Rendering"))
			{
				ImGui::Checkbox("Anti-aliased lines", &style.AntiAliasedLines);
				ImGui::SameLine();
				HelpMarker("When disabling anti-aliasing lines, you'll probably want to disable borders in your style as well.");

				ImGui::Checkbox("Anti-aliased lines use texture", &style.AntiAliasedLinesUseTex);
				ImGui::SameLine();
				HelpMarker("Faster lines using texture data. Require backend to render with bilinear filtering (not point/nearest filtering).");

				ImGui::Checkbox("Anti-aliased fill", &style.AntiAliasedFill);
				ImGui::PushItemWidth(ImGui::GetFontSize() * 8);
				ImGui::DragFloat("Curve Tessellation Tolerance", &style.CurveTessellationTol, 0.02f, 0.10f, 10.0f, "%.2f");
				if (style.CurveTessellationTol < 0.10f) style.CurveTessellationTol = 0.10f;

				// When editing the "Circle Segment Max Error" value, draw a preview of its effect on auto-tessellated circles.
				ImGui::DragFloat("Circle Tessellation Max Error", &style.CircleTessellationMaxError , 0.005f, 0.10f, 5.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				const bool show_samples = ImGui::IsItemActive();
				if (show_samples)
					ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
				if (show_samples && ImGui::BeginTooltip())
				{
					ImGui::TextUnformatted("(R = radius, N = number of segments)");
					ImGui::Spacing();
					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					const float min_widget_width = ImGui::CalcTextSize("N: MMM\nR: MMM").x;
					for (int n = 0; n < 8; n++)
					{
						const float RAD_MIN = 5.0f;
						const float RAD_MAX = 70.0f;
						const float rad = RAD_MIN + (RAD_MAX - RAD_MIN) * (float)n / (8.0f - 1.0f);

						ImGui::BeginGroup();

						ImGui::Text("R: %.f\nN: %d", rad, draw_list->_CalcCircleAutoSegmentCount(rad));

						const float canvas_width = std::max(min_widget_width, rad * 2.0f);
						const float offset_x     = floorf(canvas_width * 0.5f);
						const float offset_y     = floorf(RAD_MAX);

						const ImVec2 p1 = ImGui::GetCursorScreenPos();
						draw_list->AddCircle(ImVec2(p1.x + offset_x, p1.y + offset_y), rad, ImGui::GetColorU32(ImGuiCol_Text));
						ImGui::Dummy(ImVec2(canvas_width, RAD_MAX * 2));

						/*
						const ImVec2 p2 = ImGui::GetCursorScreenPos();
						draw_list->AddCircleFilled(ImVec2(p2.x + offset_x, p2.y + offset_y), rad, ImGui::GetColorU32(ImGuiCol_Text));
						ImGui::Dummy(ImVec2(canvas_width, RAD_MAX * 2));
						*/

						ImGui::EndGroup();
						ImGui::SameLine();
					}
					ImGui::EndTooltip();
				}
				ImGui::SameLine();
				HelpMarker("When drawing circle primitives with \"num_segments == 0\" tesselation will be calculated automatically.");

				ImGui::DragFloat("Global Alpha", &style.Alpha, 0.005f, 0.20f, 1.0f, "%.2f"); // Not exposing zero here so user doesn't "lose" the UI (zero alpha clips all widgets). But application code could have a toggle to switch between zero and non-zero.
				ImGui::DragFloat("Disabled Alpha", &style.DisabledAlpha, 0.005f, 0.0f, 1.0f, "%.2f"); ImGui::SameLine(); HelpMarker("Additional alpha multiplier for disabled items (multiply over current value of Alpha).");
				ImGui::PopItemWidth();

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::PopItemWidth();
	});
	m.def("load_style_ini", [&](std::string filePath) {
		ImGui::LoadStyleFrom(filePath.c_str());
		ImPlot::StyleColorsAuto();
	},
	py::arg("file_path"));

    #pragma endregion

    #pragma region Drag n drop

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

    #pragma endregion

    #pragma region DrawLists

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

    #pragma endregion

    #pragma region VizDrawList wrapper for extra functionality

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

            //double x = trafoStack.back().m20;
            //double y = trafoStack.back().m21;

            dl.AddText(position,
                       ImGui::GetColorU32(interpretColor(color)),
                       text.c_str());

            //dl.AddText(NULL, 0.0f, position, col, text_begin, text_end);

            applyTransform(startIndex);
        }
    };

    #pragma endregion

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
