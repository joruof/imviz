import numpy as np
import build.pyimplot as plt

def main():

    xs = np.arange(0.0, 100.0, 0.1)
    ys = np.random.rand(1000)

    while plt.wait():

        if plt.figure("Test"):
            plt.plot([1, 2, 3], [1, 2, 3], "o")

        if plt.figure("Test2"):
            plt.plot(xs, ys, "-o")

if __name__ == "__main__":
    main()
