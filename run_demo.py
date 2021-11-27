import time
import demo
import traceback

import imviz as viz


if __name__ == "__main__":

    broken = False

    state = demo.State()

    while viz.wait(vsync=True):

        if viz.update_autoreload():
            broken = False

        try:
            if not broken:
                demo.main(state)
            else:
                time.sleep(0.5)
        except Exception:
            traceback.print_exc()
            broken = True
