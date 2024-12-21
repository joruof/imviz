"""
This contains functions to export plots in various formats.
"""


import io
import os
import sys
import time
import base64
import pickle
import subprocess
import numpy as np

import warnings
warnings.filterwarnings("ignore")

import matplotlib
import matplotlib.pyplot as plt

import imviz as viz


class PlotCommand:

    def __init__(self, func_name, args, kwargs):

        self.func_name = func_name
        self.args = args
        self.kwargs = kwargs

    def opt_arg(self, pos, name, default):

        if name in self.kwargs:
            arg = self.kwargs[name]
        elif len(self.args) >= pos+1:
            arg = self.args[pos]
        else:
            arg = default

        return arg


class PlotBuffer:

    plots = {}
    capture = False

    def __init__(self):

        self.capture = False

        self.export_filetype = "pdf"
        self.export_path = os.path.abspath(os.path.expanduser("~"))
        self.export_title = ""
        self.export_x_label = ""
        self.export_y_label = ""
        self.export_width = 10
        self.export_height = 6
        self.export_dpi = 300.0
        self.export_font_size = 9.0

        self.reset()

    def reset(self):

        self.commands = []
        self.limits = None
        self.flags = None


def wrap_begin(begin_func):

    def inner(*args, **kwargs):

        res = begin_func(*args, **kwargs)

        if res:
            try:
                p = PlotBuffer.plots[viz.get_plot_id()]
            except KeyError:
                p = PlotBuffer()
                PlotBuffer.plots[viz.get_plot_id()] = p
            p.reset()
            PlotBuffer.capture = p.capture

        return res

    return inner


def wrap_plot_func(plot_func):

    def inner(*args, **kwargs):

        res = plot_func(*args, **kwargs)

        if PlotBuffer.capture:
            p = PlotBuffer.plots[viz.get_plot_id()]
            p.commands.append(PlotCommand(
                plot_func.__name__, args, kwargs))

        return res

    return inner


def wrap_end(end_func):

    def inner():

        current_plot_id = viz.get_plot_id()
        if current_plot_id is None:
            end_func()
            return

        p = PlotBuffer.plots[current_plot_id]

        export_requested = False
        file_dialog_requested = False

        viz.push_override_id(current_plot_id)
        if viz.begin_popup("##PlotContext"):
            if viz.menu_item("Export"):
                export_requested = True
            viz.separator()
            viz.end_popup()

        if export_requested:
            viz.open_popup("Export plot")

        if viz.begin_popup_modal("Export plot"):
            filetypes = ["pdf", "svg", "png"]
            idx = filetypes.index(p.export_filetype)
            idx = viz.combo("filetype", filetypes, idx)
            p.export_filetype = filetypes[idx]

            if viz.button(f"{viz.Icon.FOLDER}"):
                viz.open_popup("Select export path")
            p.export_path = viz.file_dialog_popup(
                    "Select export path",
                    p.export_path,
                    "Export")
            viz.same_line()
            p.export_path = viz.input("path", p.export_path)

            path_invalid = os.path.exists(p.export_path) and not os.path.isfile(p.export_path)
            if path_invalid:
                viz.text("The selected file path is invalid", color="red")
            else:
                p.export_path = os.path.splitext(p.export_path)[0] + "." + p.export_filetype

            p.export_title = viz.autogui(p.export_title, "title") 
            p.export_x_label = viz.autogui(p.export_x_label, "x label") 
            p.export_y_label = viz.autogui(p.export_y_label, "y label") 
            p.export_width = viz.autogui(p.export_width * 2.54, "width [cm]") / 2.54
            p.export_height = viz.autogui(p.export_height * 2.54, "height [cm]") / 2.54
            p.export_dpi = viz.autogui(p.export_dpi, "dpi")
            p.export_font_size = viz.autogui(p.export_font_size, "font_size")

            viz.separator()

            if viz.button("Cancel"):
                viz.close_current_popup()
            viz.same_line()
            viz.begin_disabled(path_invalid)
            if viz.button("Ok"):
                p.capture = True
                viz.close_current_popup()
            viz.end_disabled()

            viz.end_popup()

        viz.pop_id()

        if p.capture and len(p.commands) > 0:
            p.limits = viz.get_plot_limits()
            p.flags = viz.get_plot_flags()
            p.capture = False
            export_plot(p)

        end_func()

        PlotBuffer.capture = False

    return inner


def export_cmd_drag_point(cmd, p):

    point = cmd.args[1]
    color = cmd.opt_arg(2, "color", None)
    radius = cmd.opt_arg(3, "radius", 4)

    if type(color) == float:
        color = str(color)

    plt.plot([point[0]],
             [point[1]],
             "o",
             color=color,
             markersize=radius)


def export_cmd_plot_circle(cmd, p):

    center = cmd.args[0]
    radius = cmd.args[1]
    label = cmd.opt_arg(2, "label", "")
    color = cmd.opt_arg(3, "color", None)
    segments = cmd.opt_arg(4, "segments", 36)
    line_weight = cmd.opt_arg(5, "line_weight", 1.0)

    if type(color) == float:
        color = str(color)

    xs = []
    ys = []

    steps = segments + 1
    step = np.pi * 2 / segments

    for i in np.arange(steps):
        angle = step * i
        xs.append(center[0] + radius * np.cos(angle))
        ys.append(center[1] + radius * np.sin(angle))

    plt.plot(xs,
             ys,
             label=label,
             color=color,
             linewidth=line_weight)


def export_cmd_plot_rect(cmd, p):

    position = cmd.args[0]
    size = cmd.args[1]
    label = cmd.opt_arg(2, "label", "")
    color = cmd.opt_arg(3, "color", None)
    offset = cmd.opt_arg(4, "offset", np.array((0.5, 0.5)))
    rotation = cmd.opt_arg(5, "rotation", 1.0)
    line_weight = cmd.opt_arg(6, "line_weight", 1.0)

    px = -size[0] * offset[0];
    py = -size[1] * offset[1];

    ps = np.array([[px, px + size[0], px + size[0], px, px],
                   [py, py, py + size[1], py + size[1], py]])

    c = np.cos(rotation)
    s = np.sin(rotation)

    ps = np.array([[c, -s], [s, c]]) @ ps

    plt.plot(ps[0, :] + position[0],
             ps[1, :] + position[1],
             label=label,
             color=color,
             linewidth=line_weight)


def export_cmd_plot(cmd, p):

    fmt = cmd.opt_arg(3, "fmt", "-")

    kwargs = {}
    if "color" in cmd.kwargs:
        kwargs["color"] = cmd.kwargs["color"]
    if "label" in cmd.kwargs:
        kwargs["label"] = cmd.kwargs["label"]
    if "line_weight" in cmd.kwargs:
        kwargs["linewidth"] = cmd.kwargs["line_weight"]
    if "marker_size" in cmd.kwargs:
        kwargs["markersize"] = cmd.kwargs["marker_size"]

    plt.plot(cmd.args[0],
             cmd.args[1],
             fmt,
             **kwargs)


def export_cmd_plot_image(cmd, p):
    
    img = cmd.args[1]

    x = cmd.opt_arg(2, "x", 0.0)
    y = cmd.opt_arg(3, "y", 0.0)
    width = cmd.opt_arg(4, "width", img.shape[1])
    height = cmd.opt_arg(5, "height", img.shape[0])

    interpolate = cmd.opt_arg(9, "interpolate", True)

    extent = (x, x + width, y, y + height)

    if extent[1] < p.limits[0]:
        return
    if extent[0] > p.limits[2]:
        return
    if extent[3] < p.limits[1]:
        return
    if extent[2] > p.limits[3]:
        return

    plt.imshow(img,
               extent=extent,
               interpolation="bilinear" if interpolate else "nearest")


def export_plot(p):

    matplotlib.rcParams.update({
        'font.family': 'serif',
        'font.size': p.export_font_size,
        "font.serif": 'cmr10',
        "mathtext.fontset": 'cm'
    })

    plt.figure(figsize=(p.export_width, p.export_height), dpi=p.export_dpi)

    plt.title(p.export_title)
    plt.xlabel(p.export_x_label)
    plt.ylabel(p.export_y_label)

    for c in p.commands:
        if c.func_name == "plot":
            export_cmd_plot(c, p)
        elif c.func_name == "plot_image":
            export_cmd_plot_image(c, p)
        elif c.func_name == "plot_circle":
            export_cmd_plot_circle(c, p)
        elif c.func_name == "plot_rect":
            export_cmd_plot_rect(c, p)
        elif c.func_name == "drag_point":
            export_cmd_drag_point(c, p)

    ax = plt.gca()

    if p.flags & viz.PlotFlags.EQUAL:
        ax.axis("equal")

    ax.set(xlim=(p.limits[0], p.limits[2]),
           ylim=(p.limits[1], p.limits[3]))

    plt.savefig(p.export_path, bbox_inches="tight")


begin_plot = wrap_begin(viz.begin_plot)
begin_figure = wrap_begin(viz.begin_figure)

end_plot = wrap_end(viz.end_plot)
end_figure = wrap_end(viz.end_figure)

plot = wrap_plot_func(viz.plot)
plot_image = wrap_plot_func(viz.plot_image)
plot_circle = wrap_plot_func(viz.plot_circle)
plot_rect = wrap_plot_func(viz.plot_rect)
drag_point = wrap_plot_func(viz.drag_point)
