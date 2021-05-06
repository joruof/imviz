import numpy as np
import pandas as pd

import build.imviz as viz

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

    while viz.wait():

        if viz.figure("Test"):
            viz.plot([1, 2, 3], [1, 2, 3], shade=[0.2, 0.1, 0.3], fmt="-o")

        if viz.figure("Test2"):
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


if __name__ == "__main__":
    main()
