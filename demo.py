import numpy as np
import pandas as pd

import os
import sys

from dataclasses import dataclass


try:

    import imviz as viz
    print("Using system imviz")

except ModuleNotFoundError:

    sys.path.append(os.path.join(os.path.dirname(
        os.path.abspath(__file__)), "build"))

    import imviz as viz
    print("Using development imviz")


"""

Inspired by the imgui/implot demo files this file demonstrates the usage of
imviz by example.

"""


class State:

    def __init__(self):

        # window state

        self.show_overlay = True
        self.show_demo = True

        # input values

        self.input_string = "This is a test"
        self.input_int = 0
        self.input_float = 0.0

        self.range = (0.0, 10.0)

        self.checkbox = False

        self.color_rgb = [1.0, 1.0, 0.0]
        self.color_rgba = [1.0, 1.0, 0.0, 0.0]

        # selection

        self.items = ["mars", "venus", "apollo", "zeus", "hera"]
        self.selection = ""
        self.multi_selection = []

        # dataframes

        self.xs = np.arange(0.0, 100.0, 0.1)
        self.ys = np.random.rand(1000)

        self.frame_selection = []
        self.frame = pd.DataFrame({"xs": self.xs, "ys": self.ys})

        # plotting

        self.img = np.random.rand(240, 320, 3).astype("float32")
        self.target_pos = (0.0, 0.0)

        self.drag_point = (0, 1)
        self.drag_vline = 2.0
        self.drag_hline = 2.0

        self.drag_dots = []


def main():

    s = State()

    while viz.wait(vsync=True):

        for e in viz.get_key_events():
            if e.key == viz.KEY_K:
                if e.action == viz.PRESS and e.mod == viz.MOD_CONTROL:
                    print("Pressed Ctrl+K")

        # menus

        if viz.begin_main_menu_bar():

            if viz.begin_menu("File"):

                if viz.menu_item("Reset"):
                    s = State()

                viz.end_menu()

            if viz.begin_menu("Show"):

                if viz.menu_item("Show overlay",
                                 selected=s.show_overlay):
                    s.show_overlay = not s.show_overlay
                if viz.menu_item("Show demo",
                                 selected=s.show_demo):
                    s.show_demo = not s.show_demo
                viz.menu_item("Disabled demo", enabled=False)
                viz.menu_item("Shortcut demo", shortcut="Ctrl+Z")

                viz.end_menu()

            viz.end_main_menu_bar()

        # window options

        if s.show_overlay:

            if viz.begin_window("Overlay",
                                s.show_overlay,
                                position=(6, 32),
                                size=(100, 100),
                                title_bar=False,
                                move=False,
                                resize=False):

                viz.text("Overlay")

            viz.end_window()

        # widgets

        if viz.begin_window("Demo"):

            if viz.tree_node("Input"):

                s.input_string = viz.input("input string", s.input_string)
                s.input_int = viz.input("input int", s.input_int)
                s.input_float = viz.input("input float", s.input_float)

                s.checkbox = viz.checkbox("checkbox", s.checkbox)

                s.input_float = viz.slider("slider",
                                           s.input_float,
                                           0.0,
                                           1000)

                s.input_int = viz.drag("drag int",
                                       s.input_int,
                                       0.1,
                                       1,
                                       1000)

                s.input_float = viz.drag("drag float",
                                         s.input_float,
                                         0.01,
                                         0.0,
                                         1000)

                s.color_rgb = viz.color_edit("color rgb", s.color_rgb)
                s.color_rgba = viz.color_edit("color rgba", s.color_rgba)

                s.range = viz.range("range", s.range)

                viz.tree_pop()

            if viz.tree_node("Selection"):

                s.selection = viz.combo("combo", s.items, s.selection)

                s.multi_selection = viz.multiselect("multiselect",
                                                    s.items,
                                                    s.multi_selection)

                viz.tree_pop()

            if viz.tree_node("Dataframe"):

                viz.dataframe(s.frame, "dataframe", s.frame_selection)

                viz.tree_pop()

            if viz.tree_node("Plotting"):

                viz.next_plot_limits(0, 10, 0, 10, viz.ONCE)

                if viz.begin_plot("Plot"):

                    viz.plot_image("image",
                                   s.img,
                                   x=0, y=0,
                                   width=1, height=1)

                    viz.plot([1, 2, 3],
                             [1, 2, 3],
                             shade=[2, 1, 3],
                             fmt="-o",
                             label="line_with_dots")

                    viz.plot(np.array([1, 2, 3]) * 2,
                             fmt="*",
                             label="big stars",
                             marker_size=6,
                             marker_weight=2)

                    viz.plot([1, 2, 3],
                             np.array([1, 2, 3])**2,
                             line_weight=3,
                             fmt="-s",
                             label="small_squares")

                    s.drag_point = viz.drag_point("draggable",
                                                  s.drag_point,
                                                  show_label=True,
                                                  color=(1.0, 0.0, 0.0),
                                                  radius=10)

                    s.drag_vline = viz.drag_vline("vline",
                                                  s.drag_vline,
                                                  show_label=True,
                                                  color=(0.0, 1.0, 0.0),
                                                  width=2)

                    s.drag_hline = viz.drag_hline("hline",
                                                  s.drag_hline,
                                                  show_label=True,
                                                  color=(0.0, 1.0, 0.0),
                                                  width=2)

                    viz.annotate(5, 5, "foo")
                    viz.annotate(8, 5, "foo blue", color=(0.0, 0.2, 1.0))

                    if viz.begin_popup("##PlotContext"):
                        if viz.begin_menu("Create"):
                            if viz.menu_item("Drag Dot"):
                                s.drag_dots.append((1, 1))
                            viz.end_menu()
                        viz.end_popup()

                    for i in range(len(s.drag_dots)):
                        s.drag_dots[i]= viz.drag_point(f"dot_#{i}",
                                                       s.drag_dots[i],
                                                       show_label=True,
                                                       color=(1.0, 0.0, 0.0),
                                                       radius=4)

                        if viz.begin_popup_context_item():
                            if viz.menu_item("Delete"):
                                print("deleting")
                            viz.end_popup()

                    viz.end_plot()

                viz.tree_pop()

        viz.end_window()


if __name__ == "__main__":
    main()
