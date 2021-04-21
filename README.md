# imviz

Pythonic bindings for the great [imgui](https://github.com/ocornut/imgui) and
[implot](https://github.com/epezent/implot) libraries.

**This is work in progress and at a very early state.**

## What?

The goal of this project is to create bindings, which map the immediate mode
gui paradigm in a pythonic way, so that writing GUIs in python (especially
visualization heavy ones, meaning plots and stuff) becomes easy.

## Why?

While writing visualization tools for research, I (personally) found the
different python bindings of imgui/implot somewhat unsatisfactory, because:

* [pyimgui](https://github.com/hoffstadt/DearPyGui) maps the api but does not
  include implot
* [imgui\_datascience](https://github.com/pthom/imgui_datascience) seems
  abandoned and does not not include implot
* [dearpygui](https://github.com/hoffstadt/DearPyGui) includes implot, but
  does no replicate the immediate mode paradigm in python

## Who?

This is for the people who are too impatient to wait for matplotlib to render
10e5 data points. And for the people who are just completely unwilling (me) or
utterly incapable (also me) to deal with application state management in PyQt
(otherwise [pyqtgraph](http://pyqtgraph.org/) would be fine). 

## How?

[pybind11](https://github.com/pybind/pybind11), baby.
