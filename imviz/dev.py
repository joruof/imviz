import os
import sys
import datetime
import traceback

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
               "imviz.dev_main",
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
