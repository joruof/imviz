import re
import sys
import typing

import numpy as np
import imviz as viz

from imviz.storage import attrs_as_dict, ext_setattr


def render(obj,
           name="",
           path=[],
           parents=[],
           annotation=None):
    """
    This inspects the given object and renders a gui
    (with best effort) for all fields of the object.

    Rendering can be customized by defining an "__autogui__"
    method in the objects to be rendered.
    """

    name = name.replace("_", " ")

    obj_type = type(obj)

    if hasattr(obj, "__autogui__"): 
        return obj.__autogui__(
                obj,
                name=name,
                path=path,
                parents=parents,
                annotation=annotation)

    if obj is None:
        viz.text(f"{name}: None")
        return obj

    if obj_type in [int, np.uint8, np.int8]:
        if name == "":
            name = str(path[-1])
        return viz.drag(name, obj, 1.0, 0, 0)

    if obj_type in [float, np.float32, np.float64]:
        if name == "":
            name = str(path[-1])
        return viz.drag(name, obj)

    if obj_type == str:
        if name == "":
            name = str(path[-1])
        return viz.input(name, obj)

    if obj_type == bool:
        if name == "":
            name = str(path[-1])
        return viz.checkbox(name, obj)

    if obj_type == list:

        if len(name) > 0:
            tree_open = viz.tree_node(f"{name} [{len(obj)}]###{name}")

            if viz.begin_popup_context_item():
                if annotation is not None:
                    item_type = typing.get_args(annotation)[0]
                    if viz.menu_item("New"):
                        obj.append(item_type())
                        viz.set_mod(True)
                if viz.menu_item("Clear"):
                    obj.clear()
                    viz.set_mod(True)
                viz.end_popup()
        else:
            tree_open = True

        mod = False

        remove_list = []

        if tree_open:
            for i in range(len(obj)):

                node_name = str(i)
                
                if hasattr(obj[i], "name"):
                    node_name += f" {obj[i].name}"

                obj_tree_open = viz.tree_node(f"{node_name}###{i}")

                if viz.begin_popup_context_item():
                    if viz.menu_item("Remove"):
                        remove_list.append(i)
                        viz.set_mod(True)
                    viz.end_popup()

                if obj_tree_open:

                    obj[i] = render(
                            obj[i],
                            "",
                            path=[*path, i],
                            parents=[*parents, obj])

                    viz.tree_pop()

            for idx in remove_list:
                obj.pop(idx)

        if len(name) > 0 and tree_open:
            viz.tree_pop()

        return obj

    if obj_type == np.ndarray:

        if len(name) > 0:
            tree_open = viz.tree_node(f"{name} {list(obj.shape)}")

        else:
            tree_open = True

        mod = False

        if tree_open:
            if len(obj.shape) == 2 and obj.size <= 100:

                width_avail, _ = viz.get_content_region_avail()
                item_width = width_avail / obj.shape[1] - 8

                for i in range(obj.shape[0]):
                    for j in range(obj.shape[1]):

                        viz.set_next_item_width(item_width)

                        res = render(
                                obj[i][j],
                                f"###{i},{j}",
                                path=[*path, i, j],
                                parents=[*parents, obj])

                        if viz.mod():

                            mod |= True

                            if obj.flags.writeable:
                                obj[i][j] = res
                            else:
                                # find the root array to which obj belongs
                                # first remove non-writeable flag
                                root = None
                                for arr in parents[::-1]:
                                    if not isinstance(arr, np.ndarray):
                                        break
                                    root = arr
                                root.flags.writeable = True
                                # then reset dtype without metadata
                                root.dtype = np.dtype(root.dtype.type)
                                # finally recreate array
                                obj = np.array(obj)
                                obj[i][j] = res

                        if j < obj.shape[1]-1:
                            viz.same_line()
            else:
                for i in range(len(obj)):

                    res = render(
                            obj[i],
                            str(i),
                            path=[*path, i],
                            parents=[*parents, obj])

                    if viz.mod():

                        mod |= True

                        if obj.flags.writeable:
                            obj[i] = res
                        else:
                            # find the root array to which obj belongs
                            # first remove non-writeable flag
                            root = None
                            for arr in parents[::-1]:
                                if not isinstance(arr, np.ndarray):
                                    break
                                root = arr
                            root.flags.writeable = True
                            # then reset dtype without metadata
                            root.dtype = np.dtype(root.dtype.type)
                            # finally recreate array
                            obj = np.array(obj)
                            obj[i] = res

        if len(name) > 0 and tree_open:
            viz.tree_pop()

        viz.set_mod(mod)

        return obj

    # default case, generic object 

    attr_dict = attrs_as_dict(obj)

    if attr_dict == {}:
        return obj

    if len(name) > 0:
        tree_open = viz.tree_node(f"{name}")
    else:
        tree_open = True

    if hasattr(obj, "__annotations__"):
        obj_annots = obj.__annotations__
    else:
        obj_annots = {}

    if tree_open:
        for k, v in attr_dict.items():

            if k in obj_annots:
                annot = obj_annots[k]
            else:
                annot = None

            new_v = render(
                        v,
                        name=k,
                        path=[*path, k],
                        parents=[*parents, obj],
                        annotation=annot)

            try:
                ext_setattr(obj, k, new_v)
            except AttributeError:
                pass

    if len(name) > 0 and tree_open:
        viz.tree_pop()

    return obj
