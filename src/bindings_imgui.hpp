#pragma once

#include <pybind11/pybind11.h>

struct ImViz;

void loadImguiPythonBindings(pybind11::module& m, ImViz& viz);
void resetDragDrop();
