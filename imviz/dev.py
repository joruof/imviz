import os
import sys
import datetime
import traceback
import platform
from pydoc import locate

import imviz as viz
import minireload as mr


def launch(cls, func_name):

    file_path = os.path.abspath(sys.modules[cls.__module__].__file__)

    cls_name = os.path.basename(file_path).rsplit(".")[0]
    cls_name += "." + cls.__qualname__

    os.environ["PYTHONPATH"] = ":".join(
            sys.path + [os.path.dirname(file_path)])

    if platform.system() == 'Windows':
        # execlpe has different semantics on Windows
        cls = locate(cls_name)
        loop(cls, func_name)
    else:
        os.execlpe(sys.executable,
               "python3",
               "-m",
               "imviz.dev_main",
               cls_name,
               func_name,
               *sys.argv[1:],
               os.environ)


def try_load_source(path):

    try:
        with open(path) as fd:
            source = fd.readlines()
        return source
    except Exception:
        return ""


def build_launcher(cls, func_name):

    class ImvizLauncher:

        def __init__(self):

            viz.configure_ini_path(sys.modules[cls.__module__])

            self.obj = cls()
            self.func = getattr(self.obj, func_name)

            self.exc_info = None
            self.exc_frame_idx = 0
            self.exc_code = None
            self.request_scroll = False

        def loop(self):

            self.func()

        def exc_func(self, ei):

            if self.exc_info != ei:
                self.request_scroll = True
                self.exc_info = ei
                self.exc_frame_idx = max(0, len(ei.exc_frames) - 1)

                if isinstance(ei.exc, SyntaxError):
                    self.exc_code = try_load_source(ei.exc.filename)
                else:
                    self.exc_code = try_load_source(
                            ei.exc_frames[-1].filename)

            if not viz.wait():
                sys.exit()

            cx, cy = viz.get_viewport_center()
            w, h = viz.get_main_window_size()
            w *= 0.95
            h *= 0.95

            if viz.begin_window("Exception"):
                fade = min(1.0, (
                    datetime.datetime.now().timestamp()
                    - ei.exc_time.timestamp()) / 1.0)
                col = (1.0, fade, fade)

                time_string = ei.exc_time.strftime("%H:%M:%S")

                if viz.button("Reload"):
                    return True

                viz.separator()

                viz.text(f"Exception time {time_string}\n", color=col)
                viz.text(ei.exc_str, color=col)
                viz.text("\n")

                viz.separator()

                for i, f in enumerate(ei.exc_frames):
                    if viz.selectable(
                            f"{os.path.basename(f.filename)}"
                            + f" at line {f.lineno} in {f.function}",
                            i == self.exc_frame_idx):
                        self.exc_frame_idx = i
                        self.exc_code = try_load_source(f.filename)
                        self.request_scroll = True

            viz.end_window()

            if viz.begin_window("App state"):
                viz.autogui(self.obj.__dict__)
            viz.end_window()

            if isinstance(ei.exc_value, SyntaxError):
                viz.begin_window("Local variables")
                viz.end_window()

                self.render_source(
                        self.exc_code,
                        ei.exc_value.lineno-1)
            else:
                if viz.begin_window("Local variables"):
                    f_locals = ei.exc_frames[self.exc_frame_idx].frame.f_locals
                    try:
                        viz.AutoguiContext(ignore_custom=True).render(f_locals)
                    except:
                        viz.text("Could not render local variables!", color="red")
                viz.end_window()

                self.render_source(
                        self.exc_code,
                        ei.exc_frames[self.exc_frame_idx].lineno-1)

            return False

        def render_source(self, code_str, error_line):

            if viz.begin_window("Source code"):
                if viz.begin_table("code", 2,
                                   viz.TableFlags.BORDERS_INNER_V):

                    viz.table_setup_column(
                            "line_numbers",
                            viz.TableColumnFlags.WIDTH_FIXED
                            | viz.TableColumnFlags.NO_RESIZE)

                    viz.table_setup_column(
                            "source",
                            viz.TableColumnFlags.WIDTH_STRETCH)

                    for i, l in enumerate(code_str):
                        if i == error_line:
                            lineno_color = (1.0, 0.3, 0.3)
                            source_color = (1.0, 0.3, 0.3)
                            if self.request_scroll:
                                viz.set_scroll_here_y(0.5)
                                self.request_scroll = False
                        else:
                            lineno_color = (0.4, 0.4, 0.4)
                            source_color = (1.0, 1.0, 1.0)

                        viz.table_next_column()
                        viz.text(str(i+1), color=lineno_color)
                        viz.table_next_column()
                        viz.text(l, color=source_color)
                        viz.table_next_row()
                    viz.end_table()
            viz.end_window()

    return ImvizLauncher
