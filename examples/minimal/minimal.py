"""
Minimal example to get you started
"""

import sys
import imviz as viz


class Demo:
    def __autogui__(s, **kwargs):
        if not viz.wait():
            sys.exit()

        viz.set_main_window_title("Minimal Example")
        if viz.begin_window(
            "Minimal Example",
        ):
            if viz.button("Press me to print"):
                print("Button was pressed")
            viz.end_window()


def main():
    viz.dev.launch(Demo, "__autogui__")
