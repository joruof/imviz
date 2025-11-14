import os
import sys


try:
    from cppimviz import *
except ModuleNotFoundError:
    sys.path.append(os.path.join(os.path.dirname(
        os.path.abspath(__file__)), "../build"))
    from cppimviz import *


def configure_ini_path(module):

    if not hasattr(module, "__file__"):
        return

    main_file_name = os.path.basename(module.__file__).rsplit(".")[0]
    ini_path = os.path.join(
            os.path.abspath(os.path.dirname(module.__file__)),
            "." + main_file_name + ".imviz.ini")

    set_ini_path(ini_path)
    load_ini(ini_path)


import __main__
configure_ini_path(__main__)


from imviz.autogui import autogui_func as autogui
from imviz.autogui import AutoguiContext
from imviz.icon import Icon
from imviz.license import print_licenses
