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
        # viz.set_main_window_size((1024, 768))
        if viz.begin_window(
            "Minimal Example",
            # size=(1024, 768),
            # position=(0, 0),
            # resize=False,
        ):
            if viz.button("Press me to print"):
                print("Button was pressed")
            viz.end_window()


def main():
    viz.dev.launch(Demo, "__autogui__")


if __name__ == "__main__":
    main()
