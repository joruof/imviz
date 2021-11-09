"""
This contains common and (mostly) helpful utils.
"""

import inspect


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
