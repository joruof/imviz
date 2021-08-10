import numpy as np
import pandas as pd

import imviz as viz

"""

Inspired by the imgui/implot demo files this file demonstrates the usage of
imviz by example.

"""


def main():

    xs = np.arange(0.0, 100.0, 0.1)
    ys = np.random.rand(1000)

    frame = pd.DataFrame({"xs": xs, "ys": ys})

    frame_selection = []

    multi_selection = []

    test_string: str = "This is a test"
    steering_angle: float = 0.0

    asdf: bool = False

    img = np.random.rand(240, 320)
    img = img.astype("float32")

    display_width = 0.0
    display_height = 0.0

    target_pos = (0.0, 0.0)

    while viz.wait():

        if viz.figure("Test"):
            viz.plot([1, 2, 3], [1, 2, 3], shade=[0.2, 0.1, 0.3], fmt="-o")

        if viz.figure("Test2"):

            target_pos, mod = viz.drag_point("dst", target_pos, True, [1.0, 0.0, 0.0], 20)
            viz.plot(xs, ys, "-o")

        if viz.begin("Other"):

            viz.text("Test", color=(1.0, 0.0, 0.0))

            if viz.button("blub"):
                print("Test")

            test_string, mod = viz.input("InputTest", test_string)
            if mod:
                print(test_string)

            steering_angle, mod = viz.input("Input2", steering_angle)
            if mod:
                print(steering_angle)

            asdf, mod = viz.checkbox("TestBool", asdf)
            if mod:
                print(asdf)

            viz.dataframe(frame, "TestFrame", frame_selection)

            if (viz.multiselect(
                    "Select something",
                    ["values", "blub", "test"],
                    multi_selection)):

                print("Multiselection changed!")

        viz.end()

        if viz.begin("Tab Test"):
            if viz.begin_tab_bar("TabBar"):
                if viz.begin_tab_item("TestItem"):
                    viz.text("Hello")
                    viz.end_tab_item()
                if viz.begin_tab_item("TestItem2"):
                    viz.text("Hello2")
                    viz.end_tab_item()
                viz.end_tab_bar()

        viz.end()

        if viz.begin("Image test"):

            display_width, mod = viz.slider("Display Width", display_width, 0.0, 1000)
            display_height, mod = viz.slider("Display Height", display_height, 0.0, 1000)

                #viz.plot([1, 2, 3], [1, 2, 3])

                #viz.imshow("test_image",
                #           img,
                #           int(display_width),
                #           int(display_height))

        viz.end()


if __name__ == "__main__":
    main()
