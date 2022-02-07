#pragma once

#include <pybind11/pybind11.h>

struct ImViz;

void loadImplotPythonBindings(pybind11::module& m, ImViz& viz);
