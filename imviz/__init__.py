import os
import sys


try:
    from cppimviz import *
except ModuleNotFoundError:
    sys.path.append(os.path.join(os.path.dirname(
        os.path.abspath(__file__)), "../build"))
    from cppimviz import *


INITIAL_INI = """
[Window][Exception]
Pos=0,210
Size=400,400
Collapsed=0
DockId=0x00000005,0

[Window][App state]
Pos=399,0
Size=400,600
Collapsed=0
DockId=0x00000003,1

[Window][Local variables]
Pos=0,0
Size=400,200
Collapsed=0
DockId=0x00000004,0

[Window][Source code]
Pos=399,0
Size=400,600
Collapsed=0
DockId=0x00000003,0

[Docking][Data]
DockSpace     ID=0x00000001 Window=0x70D0451F Pos=0,0 Size=800,600 Split=X Selected=0x8B814D52
  DockNode    ID=0x00000002 Parent=0x00000001 SizeRef=400,600 Split=Y Selected=0xFD380CCE
    DockNode  ID=0x00000004 Parent=0x00000002 SizeRef=400,200 Selected=0xF90B12C6
    DockNode  ID=0x00000005 Parent=0x00000002 SizeRef=400,400 Selected=0xFD380CCE
  DockNode    ID=0x00000003 Parent=0x00000001 SizeRef=400,600 CentralNode=1 Selected=0x8B814D52
"""


def configure_ini_path(module):

    if not hasattr(module, "__file__"):
        return

    main_file_name = os.path.basename(module.__file__).rsplit(".")[0]
    ini_path = os.path.join(
            os.path.abspath(os.path.dirname(module.__file__)),
            "." + main_file_name + ".imviz.ini")

    set_ini_path(ini_path)
    load_ini_from_str(INITIAL_INI)
    load_ini(ini_path)


import __main__
configure_ini_path(__main__)


from imviz.common import *
from imviz.autogui import autogui_func as autogui
from imviz.autogui import AutoguiContext
from imviz.icon import Icon
from imviz.license import print_licenses

import imviz.dev
import imviz.task


from imviz.export import (
        begin_plot, 
        end_plot, 
        plot, 
        plot_image, 
        plot_circle, 
        plot_rect, 
        drag_point, 
        begin_figure, 
        end_figure
    )
