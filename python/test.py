import build.pyimplot as plt

def main():

    i = 0

    while plt.wait():
        plt.plot()
        print("Test", i)
        i += 1

if __name__ == "__main__":
    main()
