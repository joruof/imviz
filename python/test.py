import numpy as np
import pandas as pd
import build.pyimplot as vis


def main():

    xs = np.arange(0.0, 100.0, 0.1)
    ys = np.random.rand(1000)

    frame = pd.DataFrame({"xs": xs, "ys": ys})

    frame_selection = []

    test_string: str = "This is a test"
    steering_angle: float = 0.0

    asdf: bool = False

    while vis.wait():

        if vis.figure("Test"):
            vis.plot([1, 2, 3], [1, 2, 3], "o")

        if vis.figure("Test2"):
            vis.plot(xs, ys, "-o")

        if vis.begin("Other"):

            vis.text("Test", color=(1.0, 0.0, 0.0))

            if vis.button("blub"):
                print("Test")

            test_string, mod = vis.input("InputTest", test_string)
            if mod:
                print(test_string)

            steering_angle, mod = vis.input("Input2", steering_angle)
            if mod:
                print(steering_angle)

            asdf, mod = vis.checkbox("TestBool", asdf)
            if mod:
                print(asdf)

            vis.dataframe(frame, "TestFrame", frame_selection)

            vis.end()


if __name__ == "__main__":
    main()
