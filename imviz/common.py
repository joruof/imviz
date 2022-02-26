"""
This contains common and (mostly) helpful utils.
"""

import time
import inspect
import traceback

from contextlib import contextmanager

from imviz.autoreload import ModuleReloader

from concurrent.futures import ThreadPoolExecutor

import imviz as viz


class bundle(dict):
    """
    A dict, which allows dot notation access.
    """

    def __init__(self, *args, **kwargs):

        dict.__init__(self, *args, **kwargs)
        self.__dict__ = self

    def __setstate__(self, state):

        self.update(state)


STATICS = {}
"""
Contains all static function variables.
"""


def statics(**defaults):
    """
    This (kind of) emulates the behavior of c++ static function variables.

    It returns a bundle, which is unique to the calling function.
    Like static variables the bundle is persisted between function calls.

    Use with caution! This implementation is probably not very performant,
    but really useful for quick-and-dirty experimentation.
    """

    caller = inspect.stack()[1]
    func_id = hash(caller.filename + caller.function)

    try:
        sts = STATICS[func_id]
        sts = bundle({**defaults, **sts})
        STATICS[func_id] = sts
    except KeyError:
        sts = bundle(defaults)
        STATICS[func_id] = sts

    return sts


RELOADER = None
"""
Contains a global module reloader for easier access.
"""


def update_autoreload():
    """
    This checks all used python modules for changes (mtime), reloads
    the respective code, and updates existing code as far as possible.

    The "__main__" module cannot be reloaded due to python limitations.

    As this uses asynchronous modification scanning, the function
    must be called repeatedly in the main application loop.

    Returns True if at least one module was sucessfully reloaded.
    Returns False otherwise.
    """

    global RELOADER

    if RELOADER is None:
        RELOADER = ModuleReloader()

    return RELOADER.reload()


ASYNC_TASK_THREAD_POOL = ThreadPoolExecutor(32)
ASYNC_TASK_FUTURES = {}
ASYNC_TASK_RESULTS = {}
ASYNC_TASK_UPDATED = False
ASYNC_TASK_UPDATED = False


def update_task(name, func, *args, **kwargs):
    """
    This calls the given function in a background thread
    and returns either the latest computation result or None,
    if the task was never completed before.

    Multiple calls to this function will not queue tasks.

    If the task is currently running, no new task will be started,
    nor will the running task be interrupted.

    Task names must be unique. Two calls with the same argument
    "name" will refer to the same task.
    """

    global ASYNC_TASK_UPDATED
    ASYNC_TASK_UPDATED = False

    try:
        task_future = ASYNC_TASK_FUTURES[name]
    except KeyError:
        task_future = None

    if task_future is None:
        ASYNC_TASK_FUTURES[name] = ASYNC_TASK_THREAD_POOL.submit(
                func, *args, **kwargs)
    elif task_future.done():
        ASYNC_TASK_UPDATED = True
        ASYNC_TASK_FUTURES[name] = None
        ASYNC_TASK_RESULTS[name] = task_future.result()
        ASYNC_TASK_FUTURES[name] = ASYNC_TASK_THREAD_POOL.submit(
                func, *args, **kwargs)

    try:
        res = ASYNC_TASK_RESULTS[name]
    except KeyError:
        res = None

    return res


def current_task_updated():

    return ASYNC_TASK_UPDATED


def cancel_task(name):

    try:
        task_future = ASYNC_TASK_FUTURES[name]
        task_future.cancel()
    except KeyError:
        pass


def get_task_active(name):

    try:
        task_future = ASYNC_TASK_FUTURES[name]
        if task_future is None:
            return False
        else:
            return not task_future.done()
    except KeyError:
        return False


@contextmanager
def error_sink():
    """
    This contextmanager catches and visualizes exceptions in the gui,
    instead of handing them over to the caller.
    """

    try:
        yield
    except Exception as e:
        viz.text(f"{type(e).__name__}: {e}", color=(1, 0, 0))
        if viz.is_item_hovered():
            viz.begin_tooltip()
            viz.text(f"{traceback.format_exc(-1)}")
            viz.end_tooltip()


AUTOSAVE_REQ = False
AUTOSAVE_TIME = -1.0


@contextmanager
def autosave(obj, path=".imviz_save", timeout=0.5):

    global AUTOSAVE_REQ
    global AUTOSAVE_TIME

    if AUTOSAVE_TIME < 0:
        viz.storage.load(obj, path)
        AUTOSAVE_TIME = time.time()

    viz.push_mod_any()

    yield

    if viz.pop_mod_any():
        AUTOSAVE_REQ = True
        AUTOSAVE_TIME = time.time()

    if AUTOSAVE_REQ and (time.time() - AUTOSAVE_TIME) > timeout:
        AUTOSAVE_REQ = False
        viz.storage.save(obj, path)
