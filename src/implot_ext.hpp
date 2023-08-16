#include "binding_helpers.hpp"

namespace ImPlot {

void customPlot(
        const char* label,
        PlotArrayInfo& pai,
        py::handle color,
        bool noLine = false,
        ImPlotFlags flags = ImPlotFlags_None);

}
