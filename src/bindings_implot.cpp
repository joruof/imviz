#include "bindings_implot.hpp"

#include "binding_helpers.hpp"
#include "imviz.hpp"

#define _USE_MATH_DEFINES
#include <cmath>
#include "implot_internal.h"
#include <imgui.h>
#include <implot.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

void loadImplotPythonBindings(pybind11::module& m, ImViz& viz) {

    /**
     * Flags and defines
     */

    py::enum_<ImAxis_>(m, "Axis", py::arithmetic())
        .value("X1", ImAxis_X1)
        .value("X2", ImAxis_X2)
        .value("X3", ImAxis_X3)
        .value("Y1", ImAxis_Y1)
        .value("Y2", ImAxis_Y2)
        .value("Y3", ImAxis_Y3);

    py::enum_<ImPlotFlags_>(m, "PlotFlags", py::arithmetic())
        .value("NONE", ImPlotFlags_None)
        .value("NO_TITLE", ImPlotFlags_NoTitle)
        .value("NO_LEGEND", ImPlotFlags_NoLegend)
        .value("NO_MOUSE_TEXT", ImPlotFlags_NoMouseText)
        .value("NO_INPUTS", ImPlotFlags_NoInputs)
        .value("NO_MENUS", ImPlotFlags_NoMenus)
        .value("NO_BOX_SELECT", ImPlotFlags_NoBoxSelect)
        .value("NO_CHILD", ImPlotFlags_NoChild)
        .value("EQUAL", ImPlotFlags_Equal)
        .value("CROSSHAIRS", ImPlotFlags_Crosshairs)
        .value("ANTI_ALIASED", ImPlotFlags_AntiAliased)
        .value("CANVAS_ONLY", ImPlotFlags_CanvasOnly);

    py::enum_<ImPlotAxisFlags_>(m, "PlotAxisFlags", py::arithmetic())
        .value("NONE", ImPlotAxisFlags_None)
        .value("NO_LABEL", ImPlotAxisFlags_NoLabel)
        .value("NO_GRID_LINES", ImPlotAxisFlags_NoGridLines)
        .value("NO_TICK_MARKS", ImPlotAxisFlags_NoTickMarks)
        .value("NO_TICK_LABELS", ImPlotAxisFlags_NoTickLabels)
        .value("NO_INITIAL_FIT", ImPlotAxisFlags_NoInitialFit)
        .value("NO_MENUS", ImPlotAxisFlags_NoMenus)
        .value("OPPOSITE", ImPlotAxisFlags_Opposite)
        .value("FOREGROUND", ImPlotAxisFlags_Foreground)
        .value("LOG_SCALE", ImPlotAxisFlags_LogScale)
        .value("TIME", ImPlotAxisFlags_Time)
        .value("INVERT", ImPlotAxisFlags_Invert)
        .value("AUTO_FIT", ImPlotAxisFlags_AutoFit)
        .value("RANGE_FIT", ImPlotAxisFlags_RangeFit)
        .value("LOCK_MIN", ImPlotAxisFlags_LockMin)
        .value("LOCK_MAX", ImPlotAxisFlags_LockMax)
        .value("LOCK", ImPlotAxisFlags_Lock)
        .value("NO_DECORATIONS", ImPlotAxisFlags_NoDecorations)
        .value("AUX_DEFAULT", ImPlotAxisFlags_AuxDefault);

    py::enum_<ImPlotSubplotFlags_>(m, "PlotSubplotFlags", py::arithmetic())
        .value("NONE", ImPlotSubplotFlags_None)
        .value("NO_TITLE", ImPlotSubplotFlags_NoTitle)
        .value("NO_LEGEND", ImPlotSubplotFlags_NoLegend)
        .value("NO_MENUS", ImPlotSubplotFlags_NoMenus)
        .value("NO_RESIZE", ImPlotSubplotFlags_NoResize)
        .value("NO_ALIGN", ImPlotSubplotFlags_NoAlign)
        .value("SHARE_ITEMS", ImPlotSubplotFlags_ShareItems)
        .value("LINK_ROWS", ImPlotSubplotFlags_LinkRows)
        .value("LINK_COLS", ImPlotSubplotFlags_LinkCols)
        .value("LINK_ALL_X", ImPlotSubplotFlags_LinkAllX)
        .value("LINK_ALL_Y", ImPlotSubplotFlags_LinkAllY)
        .value("COL_MAJOR", ImPlotSubplotFlags_ColMajor);

    py::enum_<ImPlotLegendFlags_>(m, "PlotLegendFlags", py::arithmetic())
        .value("NONE", ImPlotLegendFlags_None)
        .value("NO_BUTTONS", ImPlotLegendFlags_NoButtons)
        .value("NO_HIGHLIGHT_ITEM", ImPlotLegendFlags_NoHighlightItem)
        .value("NO_HIGHLIGHT_AXIS", ImPlotLegendFlags_NoHighlightAxis)
        .value("NO_MENUS", ImPlotLegendFlags_NoMenus)
        .value("OUTSIDE", ImPlotLegendFlags_Outside)
        .value("HORIZONTAL", ImPlotLegendFlags_Horizontal);

    py::enum_<ImPlotMouseTextFlags_>(m, "PlotMouseTextFlags", py::arithmetic())
        .value("NONE", ImPlotMouseTextFlags_None)
        .value("NO_AUX_AXES", ImPlotMouseTextFlags_NoAuxAxes)
        .value("NO_FORMAT", ImPlotMouseTextFlags_NoFormat)
        .value("SHOW_ALWAYS", ImPlotMouseTextFlags_ShowAlways);

    py::enum_<ImPlotDragToolFlags_>(m, "PlotDragToolFlags", py::arithmetic())
        .value("NONE", ImPlotDragToolFlags_None)
        .value("NO_CURSORS", ImPlotDragToolFlags_NoCursors)
        .value("NO_FIT", ImPlotDragToolFlags_NoFit)
        .value("NO_INPUTS", ImPlotDragToolFlags_NoInputs)
        .value("DELAYED", ImPlotDragToolFlags_Delayed);

    py::enum_<ImPlotCond_>(m, "PlotCond")
        .value("NONE", ImPlotCond_None)
        .value("ALWAYS", ImPlotCond_Always)
        .value("ONCE", ImPlotCond_Once);

    py::enum_<ImPlotCol_>(m, "PlotCol")
        .value("LINE", ImPlotCol_Line)
        .value("FILL", ImPlotCol_Fill)
        .value("MARKER_OUTLINE", ImPlotCol_MarkerOutline)
        .value("MARKER_FILL", ImPlotCol_MarkerFill)
        .value("ERROR_BAR", ImPlotCol_ErrorBar)
        .value("FRAME_BG", ImPlotCol_FrameBg)
        .value("PLOT_BG", ImPlotCol_PlotBg)
        .value("PLOT_BORDER", ImPlotCol_PlotBorder)
        .value("LEGEND_BG", ImPlotCol_LegendBg)
        .value("LEGEND_BORDER", ImPlotCol_LegendBorder)
        .value("LEGEND_TEXT", ImPlotCol_LegendText)
        .value("TITLE_TEXT", ImPlotCol_TitleText)
        .value("INLAY_TEXT", ImPlotCol_InlayText)
        .value("AXIS_TEXT", ImPlotCol_AxisText)
        .value("AXIS_GRID", ImPlotCol_AxisGrid)
        .value("AXIS_BG", ImPlotCol_AxisBg)
        .value("AXIS_BG_HOVERED", ImPlotCol_AxisBgHovered)
        .value("AXIS_BG_ACTIVE", ImPlotCol_AxisBgActive)
        .value("SELECTION", ImPlotCol_Selection)
        .value("CROSSHAIRS", ImPlotCol_Crosshairs);

    py::enum_<ImPlotStyleVar_>(m, "PlotStyleVar")
        .value("LINE_WEIGHT", ImPlotStyleVar_LineWeight)
        .value("MARKER", ImPlotStyleVar_Marker)
        .value("MARKER_SIZE", ImPlotStyleVar_MarkerSize)
        .value("MARKER_WEIGHT", ImPlotStyleVar_MarkerWeight)
        .value("FILL_ALPHA", ImPlotStyleVar_FillAlpha)
        .value("ERROR_BAR_SIZE", ImPlotStyleVar_ErrorBarSize)
        .value("ERROR_BAR_WEIGHT", ImPlotStyleVar_ErrorBarWeight)
        .value("DIGITAL_BIT_HEIGHT", ImPlotStyleVar_DigitalBitHeight)
        .value("DIGITAL_BIT_GAP", ImPlotStyleVar_DigitalBitGap)
        .value("PLOT_BORDER_SIZE", ImPlotStyleVar_PlotBorderSize)
        .value("MINOR_ALPHA", ImPlotStyleVar_MinorAlpha)
        .value("MAJOR_TICK_LEN", ImPlotStyleVar_MajorTickLen)
        .value("MINOR_TICK_LEN", ImPlotStyleVar_MinorTickLen)
        .value("MAJOR_TICK_SIZE", ImPlotStyleVar_MajorTickSize)
        .value("MINOR_TICK_SIZE", ImPlotStyleVar_MinorTickSize)
        .value("PLOT_PADDING", ImPlotStyleVar_PlotPadding)
        .value("LABEL_PADDING", ImPlotStyleVar_LabelPadding)
        .value("LEGEND_PADDING", ImPlotStyleVar_LegendPadding)
        .value("LEGEND_INNER_PADDING", ImPlotStyleVar_LegendInnerPadding)
        .value("LEGEND_SPACING", ImPlotStyleVar_LegendSpacing)
        .value("MOUSE_POS_PADDING", ImPlotStyleVar_MousePosPadding)
        .value("ANNOTATION_PADDING", ImPlotStyleVar_AnnotationPadding)
        .value("FIT_PADDING", ImPlotStyleVar_FitPadding)
        .value("PLOT_DEFAULT_SIZE", ImPlotStyleVar_PlotDefaultSize)
        .value("PLOT_MIN_SIZE", ImPlotStyleVar_PlotMinSize);

    py::enum_<ImPlotMarker_>(m, "PlotMarker")
        .value("NONE", ImPlotMarker_None)
        .value("CIRCLE", ImPlotMarker_Circle)
        .value("SQUARE", ImPlotMarker_Square)
        .value("DIAMOND", ImPlotMarker_Diamond)
        .value("UP", ImPlotMarker_Up)
        .value("DOWN", ImPlotMarker_Down)
        .value("LEFT", ImPlotMarker_Left)
        .value("RIGHT", ImPlotMarker_Right)
        .value("CROSS", ImPlotMarker_Cross)
        .value("PLUS", ImPlotMarker_Plus)
        .value("ASTERISK", ImPlotMarker_Asterisk);

    py::enum_<ImPlotColormap_>(m, "PlotColormap")
        .value("DEEP", ImPlotColormap_Deep)
        .value("DARK", ImPlotColormap_Dark)
        .value("PASTEL", ImPlotColormap_Pastel)
        .value("PAIRED", ImPlotColormap_Paired)
        .value("VIVIDRIS", ImPlotColormap_Viridis)
        .value("PLASMA", ImPlotColormap_Plasma)
        .value("HOT", ImPlotColormap_Hot)
        .value("COOL", ImPlotColormap_Cool)
        .value("PINK", ImPlotColormap_Pink)
        .value("JET", ImPlotColormap_Jet)
        .value("TWILIGHT", ImPlotColormap_Twilight)
        .value("RDBU", ImPlotColormap_RdBu)
        .value("PIYG", ImPlotColormap_PiYG)
        .value("SPECTRAL", ImPlotColormap_Spectral)
        .value("GREYS", ImPlotColormap_Greys);

    py::enum_<ImPlotLocation_>(m, "PlotLocation")
        .value("CENTER", ImPlotLocation_Center)
        .value("NORTH", ImPlotLocation_North)
        .value("SOUTH", ImPlotLocation_South)
        .value("WEST", ImPlotLocation_West)
        .value("EAST", ImPlotLocation_East)
        .value("NORTH_WEST", ImPlotLocation_NorthWest)
        .value("NORTH_EAST", ImPlotLocation_NorthEast)
        .value("SOUTH_WEST", ImPlotLocation_SouthWest)
        .value("SOUTH_EAST", ImPlotLocation_SouthEast);

    py::enum_<ImPlotBin_>(m, "PlotBin")
        .value("SQRT", ImPlotBin_Sqrt)
        .value("STURGES", ImPlotBin_Sturges)
        .value("RICE", ImPlotBin_Rice)
        .value("SCOTT", ImPlotBin_Scott);

    /*
     * ImPlot functions
     */

    m.def("begin_figure", [&](std::string label,
                            array_like<float> size,
                            ImPlotFlags flags) {

        viz.currentWindowOpen = true;
        bool windowOpen = ImGui::Begin(label.c_str(), &viz.currentWindowOpen);

        ImVec2 plotSize = ImGui::GetContentRegionAvail();

        if (size.shape()[0] > 0) {
            assert_shape(size, {{2}});
            const float* data = size.data();
            plotSize = ImVec2(data[0], data[1]);
        } 

        viz.figurePlotOpen = ImPlot::BeginPlot(label.c_str(), plotSize, flags);

        return windowOpen && viz.figurePlotOpen;
    },
    py::arg("label") = "",
    py::arg("size") = py::array_t<float>(),
    py::arg("flags") = ImPlotFlags_None);

    m.def("end_figure", [&] () {

        if (viz.figurePlotOpen) {
            ImPlot::EndPlot();
        }

        ImGui::End();
    });

    m.def("begin_plot", [&](std::string label,
                            array_like<float> size,
                            ImPlotFlags flags) {

        ImVec2 plotSize = ImGui::GetContentRegionAvail();

        if (size.shape()[0] > 0) {
            assert_shape(size, {{2}});
            const float* data = size.data();
            plotSize = ImVec2(data[0], data[1]);
        } 

        return ImPlot::BeginPlot(label.c_str(), plotSize, flags);
    },
    py::arg("label"),
    py::arg("size") = py::array_t<float>(),
    py::arg("flags") = ImPlotFlags_None);

    m.def("end_plot", &ImPlot::EndPlot);

    m.def("begin_subplots", [&](std::string label,
                                int rows,
                                int cols,
                                array_like<float> size,
                                ImPlotSubplotFlags flags) {

        ImVec2 plotSize(-1, 0);

        if (size.shape()[0] > 0) {
            assert_shape(size, {{2}});
            const float* data = size.data();
            plotSize = ImVec2(data[0], data[1]);
        }

        return ImPlot::BeginSubplots(label.c_str(),
                                     rows,
                                     cols,
                                     plotSize,
                                     flags);
    },
    py::arg("label"),
    py::arg("rows"),
    py::arg("cols"),
    py::arg("size") = py::array(),
    py::arg("flags") = ImPlotSubplotFlags_None);

    m.def("end_subplots", ImPlot::EndSubplots);

    m.def("setup_axis", [](ImAxis axis, std::string label, ImPlotAxisFlags flags){ 

        ImPlot::SetupAxis(
            axis,
            label.empty() ? NULL : label.c_str(),
            flags);
    },
    py::arg("axis"),
    py::arg("label") = "",
    py::arg("flags") = ImPlotAxisFlags_None);

    m.def("setup_axis_limits", [](ImAxis axis, double min, double max, ImPlotCond cond){ 

        ImPlot::SetupAxisLimits(axis, min, max, cond);
    },
    py::arg("axis"),
    py::arg("min"),
    py::arg("max"),
    py::arg("flags") = ImPlotCond_Once);

    /**
     * TODO: It would be better to let the python side pass a callback,
     * which does the formatting.
     *
     * Could not yet figure out how to do this properly.
     */

    m.def("setup_axis_format", [](ImAxis axis, std::string fmt){ 

        ImPlot::SetupAxisFormat(axis, fmt.c_str());
    },
    py::arg("axis"),
    py::arg("fmt"));

    m.def("setup_legend", [](ImPlotLocation location, ImPlotLegendFlags flags){

        ImPlot::SetupLegend(location, flags);
    },
    py::arg("location") = ImPlotLocation_NorthWest,
    py::arg("flags") = ImPlotLegendFlags_None);

    m.def("setup_mouse_text", [](ImPlotLocation location, ImPlotMouseTextFlags flags){

        ImPlot::SetupMouseText(location, flags);
    },
    py::arg("location") = ImPlotLocation_SouthEast,
    py::arg("flags") = ImPlotMouseTextFlags_None);

    m.def("setup_axes", [](std::string xLabel,
                           std::string yLabel,
                           ImPlotAxisFlags xFlags,
                           ImPlotAxisFlags yFlags) { 

        ImPlot::SetupAxes(xLabel.c_str(), yLabel.c_str(), xFlags, yFlags);
    },
    py::arg("x_label"),
    py::arg("y_label"),
    py::arg("x_flags") = ImPlotAxisFlags_None,
    py::arg("y_flags") = ImPlotAxisFlags_None);

    m.def("setup_axes_limits", [](double xMin,
                                  double xMax,
                                  double yMin,
                                  double yMax,
                                  ImPlotCond cond) { 

        ImPlot::SetupAxesLimits(xMin, xMax, yMin, yMax, cond);
    },
    py::arg("x_min"),
    py::arg("x_max"),
    py::arg("y_min"),
    py::arg("y_max"),
    py::arg("cond") = ImPlotCond_Once);

    //// Sets an axis' ticks and optionally the labels. To keep the default ticks, set #keep_default=true.
    //IMPLOT_API void SetupAxisTicks(ImAxis axis, const double* values, int n_ticks, const char* const labels[] = NULL, bool keep_default = false);
    //// Sets an axis' ticks and optionally the labels for the next plot. To keep the default ticks, set #keep_default=true.
    //IMPLOT_API void SetupAxisTicks(ImAxis axis, double v_min, double v_max, int n_ticks, const char* const labels[] = NULL, bool keep_default = false);

    //// Sets the primary X and Y axes range limits. If ImPlotCond_Always is used, the axes limits will be locked (shorthand for two calls to SetupAxisLimits).
    //IMPLOT_API void SetupAxesLimits(double x_min, double x_max, double y_min, double y_max, ImPlotCond cond = ImPlotCond_Once);

    m.def("setup_finish", &ImPlot::SetupFinish);

    m.def("set_axis", &ImPlot::SetAxis);
    m.def("set_axes", &ImPlot::SetAxes);

    m.def("plot", [&](array_like<double> x,
                      array_like<double> y,
                      std::string fmt,
                      std::string label,
                      py::handle color,
                      array_like<double> shade,
                      float shadeAlpha,
                      float lineWeight,
                      float markerSize,
                      float markerWeight) {

        // interpret data

        PlotArrayInfo pai = interpretPlotArrays(x, y);

        // interpret marker format

        static std::regex re{"(-)?(o|s|d|\\*|\\+)?"};

        std::smatch match;
        std::regex_search(fmt, match, re);

        std::vector<std::string> groups;
        for (auto m : match) {
            if (m.matched) {
                groups.push_back(m.str());
            } else {
                groups.push_back("");
            }
        }

        if (groups[2] == "o") {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Circle);
        } else if (groups[2] == "s") {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Square);
        } else if (groups[2] == "d") {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Diamond);
        } else if (groups[2] == "+") {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Cross);
        } else if (groups[2] == "*") {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Asterisk);
        } else {
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_None);
        }

        // set style vars

        ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, lineWeight);
        ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, markerSize);
        ImPlot::PushStyleVar(ImPlotStyleVar_MarkerWeight, markerWeight);

        ImPlot::SetNextLineStyle(interpretColor(color), lineWeight);

        // plot lines and markers

        if (groups[1] == "-") {
            ImPlot::PlotLine(label.c_str(), pai.xDataPtr, pai.yDataPtr, pai.count);
        } else {
            ImPlot::PlotScatter(label.c_str(), pai.xDataPtr, pai.yDataPtr, pai.count);
        }

        // plot shade if needed

        size_t shadeCount = std::min(pai.count, (size_t)shade.shape()[0]);

        if (shadeCount != 0) {
            if (1 == shade.ndim()) {
                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, shadeAlpha);
                auto mean = py::cast<py::array_t<double>>(y[py::slice(0, shadeCount, 1)]);
                py::array_t<double> upper = mean + shade;
                py::array_t<double> lower = mean - shade;
                ImPlot::PlotShaded(label.c_str(),
                                   pai.xDataPtr,
                                   lower.data(),
                                   upper.data(),
                                   shadeCount);
                ImPlot::PopStyleVar();
            }
        }

        ImPlot::PopStyleVar(4);
    },
    py::arg("x"),
    py::arg("y") = py::array(),
    py::arg("fmt") = "-",
    py::arg("label") = "",
    py::arg("color") = py::array(),
    py::arg("shade") = py::array(),
    py::arg("shade_alpha") = 0.3f,
    py::arg("line_weight") = 1.0f, 
    py::arg("marker_size") = 4.0f, 
    py::arg("marker_weight") = 1.0f);

    m.def("plot_bars", [&](array_like<double> x,
                           array_like<double> y,
                           std::string label,
                           double width,
                           double shift,
                           bool horizontal) {

        PlotArrayInfo pai = interpretPlotArrays(x, y);

        if (horizontal) {
            ImPlot::PlotBarsH(
                    label.c_str(),
                    pai.xDataPtr,
                    pai.yDataPtr,
                    pai.count,
                    width,
                    shift);
        } else {
            ImPlot::PlotBars(
                    label.c_str(),
                    pai.xDataPtr,
                    pai.yDataPtr,
                    pai.count,
                    width,
                    shift);
        }
    },
    py::arg("x"),
    py::arg("y") = py::array(),
    py::arg("label") = "",
    py::arg("width") = 0.5,
    py::arg("shift") = 0.0,
    py::arg("horizontal") = false);

    m.def("plot_image", [&](
                std::string label,
                py::array& image,
                double x,
                double y,
                double displayWidth,
                double displayHeight) {

        ImageInfo info = interpretImage(image);
        
        if (displayWidth < 0) {
            displayWidth = info.imageWidth;
        }
        if (displayHeight < 0) {
            displayHeight = info.imageHeight;
        }

        GLuint textureId = uploadImage(label, info, image);

        ImPlotPoint boundsMin(x, y);
        ImPlotPoint boundsMax(x + displayWidth, y + displayHeight);

        ImPlot::PlotImage(
                label.c_str(),
                (void*)(intptr_t)textureId,
                boundsMin,
                boundsMax);
    },
    py::arg("label"),
    py::arg("image"),
    py::arg("x") = 0,
    py::arg("y") = 0,
    py::arg("width") = -1,
    py::arg("height") = -1);

    m.def("plot_image_texture", [&](
                std::string label,
                GLuint textureId,
                double x,
                double y,
                double displayWidth,
                double displayHeight) {

        ImPlotPoint boundsMin(x, y);
        ImPlotPoint boundsMax(x + displayWidth, y + displayHeight);

        ImPlot::PlotImage(
                label.c_str(),
                (void*)(intptr_t)textureId,
                boundsMin,
                boundsMax);
    },
    py::arg("label"),
    py::arg("texture_id"),
    py::arg("x") = 0,
    py::arg("y") = 0,
    py::arg("width") = -1,
    py::arg("height") = -1);

    m.def("drag_point", [&](std::string label,
                            array_like<double> point,
                            py::handle color,
                            double radius,
                            ImPlotDragToolFlags flags) {

        double x = point.data()[0];
        double y = point.data()[1];

        ImVec4 c = interpretColor(color);

        bool mod = ImPlot::DragPoint(
                ImGui::GetID(label.c_str()), &x, &y, c, radius, flags);
        viz.setMod(mod);

        return py::make_tuple(x, y);
    },
    py::arg("label"),
    py::arg("point"),
    py::arg("color") = py::array_t<double>(),
    py::arg("radius") = 4.0,
    py::arg("flags") = ImPlotDragToolFlags_None);

    m.def("plot_vlines", [&](std::string label,
                            array_like<double> xs,
                            py::handle color,
                            float width) {

        assert_shape(xs, {{-1}});

        ImVec4 c = interpretColor(color);

        ImPlot::SetNextLineStyle(c, width);

        ImPlot::PlotVLines(label.c_str(), xs.data(), xs.shape(0));
    },
    py::arg("label"),
    py::arg("xs"),
    py::arg("color") = py::array_t<double>(),
    py::arg("width") = 1.0);

    m.def("plot_hlines", [&](std::string label,
                            array_like<double> ys,
                            py::handle color,
                            float width) {

        assert_shape(ys, {{-1}});

        ImVec4 c = interpretColor(color);

        ImPlot::SetNextLineStyle(c, width);

        ImPlot::PlotHLines(label.c_str(), ys.data(), ys.shape(0));
    },
    py::arg("label"),
    py::arg("ys"),
    py::arg("color") = py::array_t<double>(),
    py::arg("width") = 1.0);

    m.def("drag_vline", [&](std::string label,
                            double x,
                            py::handle color,
                            double width) {

        ImVec4 c = interpretColor(color);

        bool mod = ImPlot::DragLineX(ImGui::GetID(label.c_str()), &x, c, width);
        viz.setMod(mod);

        return x;
    },
    py::arg("label"),
    py::arg("x"),
    py::arg("color") = py::array_t<double>(),
    py::arg("width") = 1.0);

    m.def("drag_hline", [&](std::string label,
                            double y,
                            py::handle color,
                            double width) {

        ImVec4 c = interpretColor(color);

        bool mod = ImPlot::DragLineY(ImGui::GetID(label.c_str()), &y, c, width);
        viz.setMod(mod);

        return y;
    },
    py::arg("label"),
    py::arg("y"),
    py::arg("color") = py::array_t<double>(),
    py::arg("width") = 1.0);

    m.def("drag_rect", [&](std::string label,
                            array_like<double> rect,
                            py::handle color) {

        assert_shape(rect, {{4}});

        ImVec4 c = interpretColor(color);

        bool mod = ImPlot::DragRect(
                ImGui::GetID(label.c_str()),
                rect.mutable_data(0),
                rect.mutable_data(1),
                rect.mutable_data(2),
                rect.mutable_data(3),
                c, 
                ImPlotDragToolFlags_None);

        viz.setMod(mod);

        return rect;
    },
    py::arg("label"),
    py::arg("rect"),
    py::arg("color") = py::array_t<double>());

    m.def("plot_annotation", [&](
                double x,
                double y,
                std::string text,
                py::handle color,
                array_like<double> offset,
                bool clamp) {

        ImVec4 col = interpretColor(color);
        if (col.w < 0) {
            col = ImVec4(1.0, 1.0, 1.0, 0.0);
        }

        ImVec2 o;

        if (offset.shape(0) > 0) {
            assert_shape(offset, {{2}});
            o.x = offset.data()[0];
            o.y = offset.data()[1];
        }

        ImPlot::Annotation(x, y, col, o, clamp, "%s", text.c_str());
    },
    py::arg("x"),
    py::arg("y"),
    py::arg("text"),
    py::arg("color") = py::array(),
    py::arg("offset") = py::array(),
    py::arg("clamp") = false);

    m.def("plot_rect", [&](ImPlotPoint position,
                           ImPlotPoint size,
                           std::string label,
                           py::handle color,
                           ImPlotPoint offset,
                           float rotation,
                           float lineWeight) {

        std::vector<double> xs(5);
        std::vector<double> ys(5);

        double px = -size.x * offset.x;
        double py = -size.y * offset.y;

        xs[0] = px;
        ys[0] = py;
        xs[1] = px + size.x;
        ys[1] = py;
        xs[2] = px + size.x;
        ys[2] = py + size.y;
        xs[3] = px;
        ys[3] = py + size.y;

        double s = std::sin(rotation);
        double c = std::cos(rotation);

        for (int i = 0; i < 4; ++i) {
            double tx = c * xs[i] - s * ys[i] + position.x;
            double ty = s * xs[i] + c * ys[i] + position.y;
            xs[i] = tx;
            ys[i] = ty;
        }

        xs[4] = xs[0];
        ys[4] = ys[0];

        ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, lineWeight);
        ImPlot::PushStyleColor(ImPlotCol_Line, interpretColor(color));
        ImPlot::PlotLine(label.c_str(), xs.data(), ys.data(), 5);
        ImPlot::PopStyleColor();
        ImPlot::PopStyleVar();
    },
    py::arg("position"),
    py::arg("size"),
    py::arg("label") = "",
    py::arg("color") = py::array(),
    py::arg("offset") = ImPlotPoint(0.5f, 0.5f),
    py::arg("rotation") = 0.0f,
    py::arg("line_weight") = 1.0f);

    m.def("plot_circle", [&](ImPlotPoint center,
                             double radius,
                             std::string label,
                             py::handle color,
                             size_t segments,
                             float lineWeight) {

        size_t steps = segments + 1;

        std::vector<double> xs(steps);
        std::vector<double> ys(steps);

        double step = M_PI * 2 / segments;

        for (size_t i = 0; i < steps; ++i) {
            double angle = step * i;
            xs[i] = center.x + radius * std::cos(angle);
            ys[i] = center.y + radius * std::sin(angle);
        }

        ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, lineWeight);
        ImPlot::PushStyleColor(ImPlotCol_Line, interpretColor(color));
        ImPlot::PlotLine(label.c_str(), xs.data(), ys.data(), steps);
        ImPlot::PopStyleColor();
        ImPlot::PopStyleVar();
    },
    py::arg("center"),
    py::arg("radius"),
    py::arg("label") = "",
    py::arg("color") = py::array(),
    py::arg("segments") = 36,
    py::arg("line_weight") = 1.0f);

    m.def("is_plot_selected", ImPlot::IsPlotSelected);

    m.def("get_plot_selection", [&]() {
        ImPlotRect sel = ImPlot::GetPlotSelection();
        return std::vector<double>({sel.X.Min, sel.Y.Min, sel.X.Max, sel.Y.Max});
    });

    m.def("plot_selection_ended", [&]() {
        ImPlotContext& gp = *GImPlot;
        ImGuiIO& IO = ImGui::GetIO();
        return (ImPlot::GetCurrentPlot()->Selecting
                && IO.MouseReleased[gp.InputMap.Select]);
    });

    m.def("cancel_plot_selection", ImPlot::CancelPlotSelection);

    m.def("hard_cancel_plot_selection", []() {
        ImPlot::GetCurrentPlot()->Selected = false;
        ImPlot::GetCurrentPlot()->Selecting = false;
    });

    m.def("get_plot_pos", ImPlot::GetPlotPos);
    m.def("get_plot_size", ImPlot::GetPlotSize);

    m.def("get_plot_scale", [&](){ 

        ImVec2 a = ImPlot::PlotToPixels(0.0, 0.0);
        ImVec2 b = ImPlot::PlotToPixels(10.0e7, 10.0e7);
        double scaleX = std::abs(((double)b.x - (double)a.x) / 10.0e7);
        double scaleY = std::abs(((double)b.y - (double)a.y) / 10.0e7);

        return ImPlotPoint(scaleX, scaleY);
    });

    m.def("get_inv_plot_scale", [&](){ 

        ImVec2 a = ImPlot::PlotToPixels(0.0, 0.0);
        ImVec2 b = ImPlot::PlotToPixels(10.0e7, 10.0e7);
        double scaleX = std::abs(((double)b.x - (double)a.x) / 10.0e7);
        double scaleY = std::abs(((double)b.y - (double)a.y) / 10.0e7);

        return ImPlotPoint(1.0/scaleX, 1.0/scaleY);
    });

    m.def("get_plot_limits", [&]() {
        ImPlotRect rect = ImPlot::GetPlotLimits();
        return std::vector<double>({rect.X.Min, rect.Y.Min, rect.X.Max, rect.Y.Max});
    });

    m.def("get_plot_mouse_pos", [&]() {
        return ImPlot::GetPlotMousePos();
    });

    m.def("pixels_to_plot", [&](float x, float y, ImAxis xAxis, ImAxis yAxis) {
        ImPlotPoint point = ImPlot::PixelsToPlot(x, y, xAxis, yAxis);
        return std::vector<double>({point.x, point.y});
    },
    py::arg("x"),
    py::arg("y"),
    py::arg("x_axis") = IMPLOT_AUTO,
    py::arg("y_axis") = IMPLOT_AUTO);

    m.def("plot_to_pixels", [&](double x, double y, ImAxis xAxis, ImAxis yAxis) {
        ImVec2 point = ImPlot::PlotToPixels(x, y, xAxis, yAxis);
        return std::vector<double>({point.x, point.y});
    },
    py::arg("x"),
    py::arg("y"),
    py::arg("x_axis") = IMPLOT_AUTO,
    py::arg("y_axis") = IMPLOT_AUTO);

    m.def("plot_contains", [&](ImPlotPoint point) {
        return ImPlot::GetPlotLimits().Contains(point.x, point.y);
    });

    m.def("get_plot_id", [&]() {
        ImPlotPlot* plot = ImPlot::GetCurrentPlot();
        if (plot == nullptr) {
            return py::object(py::none());
        } else {
            return py::cast(plot->ID);
        }
    });
}
