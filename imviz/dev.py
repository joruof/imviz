import os
import sys
import inspect
import datetime
import traceback
import platform
from pydoc import locate

import imviz as viz


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


def render_source(code_str, error_line, new_stack_frame_sel):

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
                    if new_stack_frame_sel:
                        viz.set_scroll_here_y(0.5)
                        new_stack_frame_sel = False
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

    return new_stack_frame_sel


def loop(cls, func_name):

    viz.configure_ini_path(sys.modules[cls.__module__])

    obj = cls()
    func = getattr(obj, func_name)

    exc_str = None
    exc_time = None
    exc_type = None
    exc_value = None
    exc_tb = None
    exc_frames = None
    exc_frame_idx = -1
    exc_code = None

    new_stack_frame_sel = False

    while True:
        try:
            if viz.update_autoreload():
                exc_str = None

            if exc_str is None:
                func()
            else:
                if not viz.wait(powersave=True):
                    sys.exit()

                cx, cy = viz.get_viewport_center()
                w, h = viz.get_main_window_size()
                w *= 0.95
                h *= 0.95

                if viz.begin_window("Exception"):

                    fade = min(1.0, (
                        datetime.datetime.now().timestamp()
                        - exc_time.timestamp()) / 1.0)
                    col = (1.0, fade, fade)

                    time_string = exc_time.strftime("%H:%M:%S")

                    if viz.button("Reload"):
                        exc_str = None
                        continue

                    viz.separator()

                    viz.text(f"Exception time {time_string}\n", color=col)
                    viz.text(exc_str, color=col)
                    viz.text("\n")

                    viz.separator()

                    for i, f in enumerate(exc_frames):
                        if viz.selectable(
                                f"{os.path.basename(f.filename)}"
                                + f" at line {f.lineno} in {f.function}",
                                i == exc_frame_idx):
                            exc_frame_idx = i
                            exc_code = try_load_source(f.filename)
                            new_stack_frame_sel = True

                viz.end_window()

                if viz.begin_window("App state"):
                    viz.autogui(obj.__dict__)
                viz.end_window()

                if isinstance(exc_value, SyntaxError):

                    viz.begin_window("Local variables")
                    viz.end_window()

                    new_stack_frame_sel = render_source(
                            exc_code,
                            exc_value.lineno-1,
                            new_stack_frame_sel)

                else:
                    if viz.begin_window("Local variables"):
                        f_locals = exc_frames[exc_frame_idx].frame.f_locals
                        try:
                            viz.AutoguiContext(ignore_custom=True).render(f_locals)
                        except:
                            viz.text("Could not render local variables!", color="red")
                    viz.end_window()

                    new_stack_frame_sel = render_source(
                            exc_code,
                            exc_frames[exc_frame_idx].lineno-1,
                            new_stack_frame_sel)

        except SystemExit:
            return
        except Exception as e:
            traceback.print_exc()

            # collect information about the exception
            exc_str = traceback.format_exc()
            exc_time = datetime.datetime.now()
            (exc_type, exc_value, exc_tb) = sys.exc_info()

            if isinstance(e, SyntaxError):
                exc_frames = []
                exc_frame_idx = 0
                exc_code = try_load_source(e.filename)
            else:
                exc_frames = inspect.getinnerframes(exc_tb)
                exc_frame_idx = max(0, len(exc_frames) - 1)
                exc_code = try_load_source(exc_frames[-1].filename)

            new_stack_frame_sel = True
