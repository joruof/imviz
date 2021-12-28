import os
import sys

try:
    from cppimviz import *
except ModuleNotFoundError:
    sys.path.append(os.path.join(os.path.dirname(
        os.path.abspath(__file__)), "../build"))
    from cppimviz import *
    print("Using development imviz")

from imviz.common import *

from imviz.autogui import render as autogui
