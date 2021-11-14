"""
This contains common and (mostly) helpful utils.
"""

import inspect

from imviz.autoreload import ModuleReloader


class bundle(dict):
    """
    A dict, which allows dot notation access.
    """

    def __init__(self, *args, **kwargs):

        dict.__init__(self, *args, **kwargs)
        self.__dict__ = self


STATICS = {}
"""
Contains all static function variables.
"""


def statics():
    """
    This (kind of) emulates the behavior of c++ static function variables.

    It returns a bundle, which is unique to the calling function.
    Like static variables the bundle is persisted between function calls.
    """

    caller = inspect.stack()[1]
    func_id = hash(caller.filename + caller.function)

    try:
        sts = STATICS[func_id]
    except KeyError:
        sts = bundle()
        STATICS[func_id] = sts

    return sts


RELOADER = ModuleReloader()
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

    return RELOADER.reload()
