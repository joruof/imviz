import time
import demo
import traceback

import imviz as viz


if __name__ == "__main__":

    while viz.wait(vsync=True):

        if viz.reload_code():
            print("Reloaded")

        try:
            demo.main()
        except Exception:
            traceback.print_exc()
            time.sleep(1)
