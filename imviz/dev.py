"""
Because the automatic code reloading cannot reload the __main__ module,
we provide a module here, which can launch and reload a given class.

This frees the library user from providing such a main module themselves.

For specific needs not covered by this simple implementation, you may use
this module as a reference for your own (development) main module.
"""

import time
import argparse
import traceback

# i like this
from pydoc import locate

import imviz as viz


def main():

    parser = argparse.ArgumentParser(
            description="Launch a class with automatic code reloading")

    parser.add_argument(
            "class_name",
            type=str,
            help="the name of the class to launch")

    args = parser.parse_args()

    broken = False

    cls = locate(args.class_name)
    if cls is None:
        print(f"Could not find class {args.class_name}")
        return

    obj = cls()

    while viz.wait(vsync=True):

        if viz.update_autoreload():
            broken = False

        try:
            if not broken:
                viz.auto_gui(obj)
            else:
                time.sleep(0.5)
        except Exception:
            traceback.print_exc()
            broken = True


if __name__ == "__main__":
    main()
