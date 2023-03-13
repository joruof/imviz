import copy
import typing
import numbers
import itertools
import traceback

import imviz as viz
import numpy as np

from imviz.storage import ext_setattr


def autogui_func(obj, name="", **kwargs):

    ctx = AutoguiContext(**kwargs)
    return ctx.render(obj, name)


def list_item_context(obj, name, ctx):

    i = ctx.path[-1]

    if viz.begin_popup_context_item():
        if viz.menu_item("Duplicate"):
            ctx.duplicate = (i, copy.deepcopy(obj))
        if viz.menu_item("Remove"):
            ctx.remove_list.append(i)
            viz.set_mod(True)
        viz.end_popup()

    ctx.post_header_hooks.remove(list_item_context)


class AutoguiContext:

    def __init__(self,
                 path=[],
                 parents=[],
                 annotation=None,
                 ignore_custom=False):

        self.path = path
        self.parents = parents
        self.annotation = annotation
        self.ignore_custom = ignore_custom

        self.post_header_hooks = []
        self.remove_list = []
        self.duplicate = (None, None)

    def try_render(self, obj, name=""):

        try:
            return self.try_render(obj, name)
        except Exception as e:
            viz.text(f"{name}: {e}")
            if viz.is_item_hovered():
                viz.begin_tooltip()
                viz.text(traceback.format_exc())
                viz.end_tooltip()

        return obj

    def call_post_header_hooks(self, obj, name):

        for h in self.post_header_hooks:
            h(obj, name, self)

    def render(self, obj, name=""):

        """
        This inspects the given object and renders a gui
        (with best effort) for all fields of the object.

        Rendering can be customized by defining an "__autogui__"
        method in the objects to be rendered.
        """

        name = name.replace("_", " ")

        obj_type = type(obj)

        if hasattr(obj, "__autogui__") and not self.ignore_custom:
            return obj.__autogui__(name, ctx=self)

        if obj is None:
            viz.text(f"{name}: None")
            self.call_post_header_hooks(obj, name)
            return obj

        if obj_type == bool or obj_type == np.bool_:
            if name == "":
                name = str(self.path[-1])
            res = viz.checkbox(name, obj)
            self.call_post_header_hooks(obj, name)
            return res

        if isinstance(obj, numbers.Integral):
            if name == "":
                name = str(self.path[-1])
            res = obj_type(viz.drag(name, obj, 1.0, 0, 0))
            self.call_post_header_hooks(obj, name)
            return res

        if isinstance(obj, numbers.Real):
            if name == "":
                name = str(self.path[-1])
            res = obj_type(viz.drag(name, obj))
            self.call_post_header_hooks(obj, name)
            return res

        if obj_type == str:
            if name == "":
                name = str(self.path[-1])
            res = viz.input(name, obj)
            self.call_post_header_hooks(obj, name)
            return res

        if obj_type == tuple:

            if len(name) > 0:
                tree_open = viz.tree_node(f"{name} [{len(obj)}]-tuple###{name}")
                self.call_post_header_hooks(obj, name)
            else:
                tree_open = True

            if tree_open:
                for i in range(len(obj)):

                    node_name = str(i)

                    if hasattr(obj[i], "name"):
                        node_name += f" {obj[i].name}"

                    obj_tree_open = viz.tree_node(f"{node_name}###{i}")

                    if obj_tree_open:

                        self.path.append(i)
                        self.parents.append(obj)

                        self.render(obj[i], "")

                        self.parents.pop()
                        self.path.pop()

                        viz.tree_pop()

            if len(name) > 0 and tree_open:
                viz.tree_pop()

            return obj

        if obj_type == list:

            if len(name) > 0:
                tree_open = viz.tree_node(f"{name} [{len(obj)}]###{name}")
                self.call_post_header_hooks(obj, name)
                if viz.begin_popup_context_item():
                    if self.annotation is not None:
                        item_type = typing.get_args(self.annotation)[0]
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
            duplicate = (None, None)

            if tree_open:
                for i in range(len(obj)):

                    node_name = str(i)

                    if hasattr(obj[i], "shape"):
                        node_name += f" {list(obj[i].shape)}"
                    if hasattr(obj[i], "name"):
                        node_name += f" {obj[i].name}"

                    self.post_header_hooks.append(list_item_context)

                    self.path.append(i)
                    self.parents.append(obj)

                    obj[i] = self.render(obj[i], node_name)

                    self.parents.pop()
                    self.path.pop()

                if self.duplicate[0] is not None:
                    obj.insert(self.duplicate[0], self.duplicate[1])
                    self.duplicate = (None, None)

                for idx in self.remove_list:
                    obj.pop(idx)

            if len(name) > 0 and tree_open:
                viz.tree_pop()

            return obj

        if hasattr(obj, "shape") and hasattr(obj, "__getitem__"):

            # to avoid many array lookups in the loop
            # we collect the requested indices in "path"
            indices = tuple(itertools.takewhile(
                    lambda x: isinstance(x[0], int) and type(x[1]) == type(obj),
                        zip(self.path[::-1], self.parents[::-1])))[::-1]
            indices = tuple(i[0] for i in indices)

            li = len(indices)

            if len(name) > 0:
                tree_open = viz.tree_node(f"{name} {list(obj.shape)[li:]}")
                self.call_post_header_hooks(obj, name)
            else:
                tree_open = True

            mod = False

            if tree_open:
                if len(obj.shape) == 2:

                    width_avail, _ = viz.get_content_region_avail()
                    item_width = max(32, width_avail / obj.shape[1] - 8)

                    # this tremendously speeds up zarr array access
                    # because we are now operating on a numpy array
                    arr_view = obj[:, :]

                    for i in range(obj.shape[0]):
                        for j in range(obj.shape[1]):

                            viz.set_next_item_width(item_width)

                            self.path.append(i)
                            self.path.append(j)
                            self.parents.append(obj)

                            res = self.render(arr_view[i, j], f"###{i},{j}")

                            if viz.is_item_hovered():
                                viz.begin_tooltip()
                                viz.text(f"({i}, {j})")
                                viz.end_tooltip()

                            self.parents.pop()
                            self.path.pop()
                            self.path.pop()

                            if viz.mod():
                                mod = True
                                obj[i, j] = res

                            if j < obj.shape[1]-1:
                                viz.same_line()
                else:
                    # to avoid many array lookups in the loop
                    # we collect the requested indices in "path"

                    if len(obj.shape) < 2:
                        arr_view = obj[:]
                        for i in range(len(arr_view)):
                            # lookup happens here

                            self.path.append(i)
                            self.parents.append(obj)

                            res = self.render(arr_view[i], str(i))

                            self.parents.pop()
                            self.path.pop()

                            if viz.mod():
                                obj[i] = res
                    elif len(obj.shape) - li == 2:
                        # lookup happens here

                        self.parents.append(obj)

                        res = self.render(obj[indices])

                        self.parents.pop()

                        if viz.mod():
                            obj[indices] = res
                    else:
                        for i in range(obj.shape[li]):

                            self.path.append(i)
                            self.parents.append(obj)

                            res = self.render(obj, str(i))

                            self.parents.pop()
                            self.path.pop()

                    if viz.mod():
                        mod = True

            if len(name) > 0 and tree_open:
                viz.tree_pop()

            viz.set_mod(mod)

            return obj

        # default case, generic object

        if hasattr(obj, "__dict__"):
            attr_dict = obj.__dict__
        elif hasattr(obj, "__slots__"):
            attr_dict = {n: getattr(obj, n) for n in obj.__slots__}
        elif isinstance(obj, dict):
            attr_dict = obj
        else:
            # weird object has no common attributes

            if hasattr(obj, "__len__") and hasattr(obj, "__getitem__"):

                # at least we we can iterate it

                if len(name) > 0:
                    tree_open = viz.tree_node(f"{name} [{len(obj)}]###{name}")
                    self.call_post_header_hooks(obj, name)
                else:
                    tree_open = True

                mod = False

                remove_list = []
                duplicate = (None, None)

                if tree_open:
                    for i in range(len(obj)):

                        node_name = str(i)

                        if hasattr(obj[i], "shape"):
                            node_name += f" {list(obj[i].shape)}"
                        if hasattr(obj[i], "name"):
                            node_name += f" {obj[i].name}"

                        obj_tree_open = viz.tree_node(f"{node_name}###{i}")

                        if viz.begin_popup_context_item():
                            if viz.menu_item("Duplicate"):
                                duplicate = (i, copy.deepcopy(obj[i]))
                            if viz.menu_item("Remove"):
                                remove_list.append(i)
                                viz.set_mod(True)
                            viz.end_popup()

                        if obj_tree_open:

                            self.path.append(i)
                            self.parents.append(obj)

                            obj[i] = self.render(obj[i], "")

                            self.parents.pop()
                            self.path.pop()

                            viz.tree_pop()

                    if duplicate[0] is not None:
                        obj.insert(duplicate[0], duplicate[1])

                    for idx in remove_list:
                        obj.pop(idx)

                if len(name) > 0 and tree_open:
                    viz.tree_pop()

                return obj
            else:
                viz.text(f"{name}: " + "???")
                return obj

        if len(name) > 0:
            tree_open = viz.tree_node(f"{name}")
            self.call_post_header_hooks(obj, name)
        else:
            tree_open = True

        obj_annots = getattr(obj, "__annotations__", {})

        if tree_open:
            for k, v in attr_dict.items():

                if k in obj_annots:
                    self.annotation = obj_annots[k]

                self.path.append(k)
                self.parents.append(obj)

                new_v = self.render(v, name=str(k))

                self.annotation = None
                self.parents.pop()
                self.path.pop()

                if viz.mod_any():
                    try:
                        ext_setattr(obj, k, new_v)
                    except AttributeError:
                        pass

        if len(name) > 0 and tree_open:
            viz.tree_pop()

        return obj
