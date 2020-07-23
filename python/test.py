import build.pyimplot as plt

def main():

    i = 0

    while plt.wait():
        plt.plot([[1, 2, 3], [0.0, 1.0, 2.0]], fmt="-s")
        plt.figure("Test")
        plt.plot([1, 2, 3], [1, 2, 3], "o")
        i += 1

if __name__ == "__main__":
    main()
