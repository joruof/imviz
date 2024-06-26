import copy
import typing
import numbers
import itertools
import traceback

import imviz as viz
import numpy as np

from objtoolbox import ext_setattr


def autogui_func(obj, name="", **params):

    return AutoguiContext(params=params).render(obj, name)


def list_item_context(obj, name, ctx):

    i = ctx.path[-1]

    if viz.get_item_id() > 0 and viz.begin_popup_context_item():
        if hasattr(ctx.parents[-1], "insert"):
            if viz.menu_item("Duplicate"):
                ctx.duplicate_item = i
        if hasattr(ctx.parents[-1], "__delitem__"):
            if viz.menu_item("Remove"):
                ctx.remove_item = i
                viz.set_mod(True)
        viz.end_popup()

    ctx.post_header_hooks.remove(list_item_context)


class AutoguiContext:

    def __init__(self,
                 params={},
                 path=[],
                 parents=[],
                 annotation=None,
                 ignore_custom=False):

        self.params = {**params}
        self.path = [*path]
        self.parents = [*parents]
        self.annotation = annotation
        self.ignore_custom = ignore_custom

        self.post_header_hooks = []
        self.remove_item = None
        self.duplicate_item = None

        self.path_of_mod_item = []

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

        obj_type = type(obj)

        if hasattr(obj, "__autogui__") and not self.ignore_custom:
            viz.push_mod_any()
            res = obj.__autogui__(name, ctx=self, **self.params)
            if viz.pop_mod_any():
                if self.path_of_mod_item == []:
                    self.path_of_mod_item = copy.deepcopy(self.path)
            return res

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

        # to reduce array lookups we collect the requested indices in "path"
        # the lookup then happens just before rendering the actual values
        indices = tuple(itertools.takewhile(
                lambda x: isinstance(x[0], int) and type(x[1]) == type(obj),
                    zip(self.path[::-1], self.parents[::-1])))[::-1]
        indices = tuple(i[0] for i in indices)
        li = len(indices)

        if len(name) > 0:
            tree_node_label = f"{name} "
            if hasattr(obj, "shape") and type(obj.shape) == tuple:
                tree_node_label += f"{list(obj.shape)[li:]}"
            elif hasattr(obj, "__len__"):
                tree_node_label += f"[{len(obj)}]"
            tree_node_label += f"###{name}"

            tree_open = viz.tree_node(tree_node_label)
            self.call_post_header_hooks(obj, name)
        else:
            tree_open = True

        if hasattr(obj, "shape") and hasattr(obj, "__getitem__"):

            if tree_open:
                if len(obj.shape) - li == 2:

                    # this tremendously speeds up zarr array access
                    # because we are now operating on a numpy array
                    arr_view = obj[indices]

                    width_avail, _ = viz.get_content_region_avail()
                    item_width = max(64, width_avail / arr_view.shape[-1] - 8)

                    for i in range(arr_view.shape[-2]):
                        for j in range(arr_view.shape[-1]):

                            viz.set_next_item_width(item_width)

                            self.path.append(i)
                            self.path.append(j)
                            self.parents.append(obj)

                            res = self.render(arr_view[i, j], f"###{i},{j}")

                            if viz.is_item_hovered():
                                viz.begin_tooltip()
                                viz.text(f"({i}, {j})")
                                viz.end_tooltip()

                            if viz.mod():
                                obj[indices + (i, j)] = res
                                if self.path_of_mod_item == []:
                                    self.path_of_mod_item = copy.deepcopy(self.path)

                            if j < arr_view.shape[-1] - 1:
                                viz.same_line()

                            self.parents.pop()
                            self.path.pop()
                            self.path.pop()

                elif len(obj.shape) - li == 1:

                    # lookup happens here
                    arr_view = obj[indices]

                    for i in range(len(arr_view)):

                        self.path.append(i)
                        self.parents.append(obj)

                        res = self.render(arr_view[i], str(i))

                        if viz.mod():
                            obj[indices + (i,)] = res
                            if self.path_of_mod_item == []:
                                self.path_of_mod_item = copy.deepcopy(self.path)

                        self.parents.pop()
                        self.path.pop()
                else:
                    for i in range(obj.shape[li]):

                        self.path.append(i)
                        self.parents.append(obj)

                        res = self.render(obj, str(i))

                        self.parents.pop()
                        self.path.pop()

            if len(name) > 0 and tree_open:
                viz.tree_pop()

            return obj

        if hasattr(obj, "__dict__"):
            attr_dict = obj.__dict__
        elif hasattr(obj, "__slots__"):
            attr_dict = {n: getattr(obj, n) for n in obj.__slots__}
        elif isinstance(obj, dict):
            attr_dict = obj
        elif isinstance(obj, set):
            attr_dict = {i: n for i, n in enumerate(obj)}
        elif hasattr(obj, "__len__") and hasattr(obj, "__getitem__"):
            if tree_open:
                for i in range(len(obj)):

                    self.post_header_hooks.append(list_item_context)

                    self.path.append(i)
                    self.parents.append(obj)

                    viz.push_mod_any()
                    res = self.render(obj[i], str(i))

                    if viz.pop_mod_any() and res is not obj[i]:
                        try:
                            obj.__setitem__(i, res)
                        except AttributeError:
                            pass
                        if self.path_of_mod_item == []:
                            self.path_of_mod_item = copy.deepcopy(self.path)

                    self.parents.pop()
                    self.path.pop()

                if self.duplicate_item is not None:
                    obj.insert(self.duplicate_item, copy.deepcopy(obj[self.duplicate_item]))
                    viz.set_mod(True)
                    self.duplicate_item = None

                if self.remove_item is not None:
                    del obj[self.remove_item]
                    viz.set_mod(True)
                    self.remove_item = None

            if len(name) > 0 and tree_open:
                viz.tree_pop()

            return obj
        else:
            if len(name) == 0:
                viz.text(f"{name}: " + "???")
            if tree_open:
                viz.tree_pop()
            return obj

        obj_annots = getattr(obj, "__annotations__", {})
    
        # default case: generic object

        if tree_open:
            for k, v in attr_dict.items():

                if k in obj_annots:
                    self.annotation = obj_annots[k]

                self.path.append(k)
                self.parents.append(obj)

                viz.push_mod_any()
                new_v = self.render(v, str(k))

                if viz.pop_mod_any() and new_v is not v:
                    try:
                        ext_setattr(obj, k, new_v)
                    except AttributeError:
                        pass
                    if self.path_of_mod_item == []:
                        self.path_of_mod_item = copy.deepcopy(self.path)

                self.annotation = None
                self.parents.pop()
                self.path.pop()

        if len(name) > 0 and tree_open:
            viz.tree_pop()

        return obj
