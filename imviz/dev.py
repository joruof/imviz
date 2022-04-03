"""
Because the automatic code reloading cannot reload the __main__ module,
we provide a module here, which can launch and reload a given class.

This frees the library user from providing such a main module themselves.

For specific needs not covered by this simple implementation, you may use
this module as a reference for your own (development) main module.
"""

import os
import sys
import datetime
import argparse
import traceback

# i like this
from pydoc import locate

import imviz as viz


def launch(cls, func_name):

    file_path = os.path.abspath(sys.modules[cls.__module__].__file__)

    cls_name = os.path.basename(file_path).rsplit(".")[0]
    cls_name += "." + cls.__qualname__

    os.environ["PYTHONPATH"] = ":".join(
            sys.path + [os.path.dirname(file_path)])

    os.execlpe("python3",
               "python3",
               "-m",
               "imviz.dev",
               cls_name,
               func_name,
               os.environ)


def loop(cls, func_name):

    viz.configure_ini_path(sys.modules[cls.__module__])

    obj = cls()
    func = getattr(obj, func_name)

    exception = None
    exce_time = None

    while True:
        try:
            if viz.update_autoreload():
                exception = None

            if exception is None:
                func()
            else:
                if not viz.wait():
                    sys.exit()

                cx, cy = viz.get_viewport_center()
                w, h = viz.get_main_window_size()
                w *= 0.95
                h *= 0.95

                if viz.begin_window("Error",
                                    position=(cx - w/2, cy - h/2),
                                    size=(w, h),
                                    title_bar=False,
                                    move=False,
                                    resize=False):

                    fade = min(1.0, (
                        datetime.datetime.now().timestamp()
                        - exce_time.timestamp()) / 1.0)
                    col = (1.0, fade, fade)

                    time_string = exce_time.strftime("%H:%M:%S")

                    viz.text(f"Exception time {time_string}\n", color=col)
                    viz.text(exception, color=col)
                    viz.text("\n")

                    viz.separator()

                    viz.autogui(obj.__dict__, "application state")

                viz.end_window()
        except SystemExit:
            return
        except Exception:
            traceback.print_exc()
            exception = traceback.format_exc()
            exce_time = datetime.datetime.now()


def main():

    parser = argparse.ArgumentParser(
            description="Launch a class with automatic code reloading")

    parser.add_argument(
            "class_name",
            type=str,
            help="the name of the class to instantiate")

    parser.add_argument(
            "func_name",
            type=str,
            help="the name of the method to call")

    args = parser.parse_args()

    cls = locate(args.class_name)

    if cls is None:
        print(f"Could not find class {args.class_name}")
        return

    loop(cls, args.func_name)


if __name__ == "__main__":
    main()
