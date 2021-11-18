"""
A very rudimentary and opinionated storage system.

The idea is not to store and load the entire object hierarchy.
Instead an already instantiated object hierarchy is update by the stored data.
This has the advantage that we can update most of the code without thinking
too much about how this will fit the already stored data.

The storage system also takes care of handling larger numpy arrays, which makes
working with images or points clouds much easier.
"""

import os
import json
import types
import hashlib
import importlib

import numpy as np


class Skip:
    """
    This signals that a value should not be serialized (only used internally).
    """
    pass


def full_type(o):
    """
    Returns the fully qualified type name of a given object.
    """

    cls = o.__class__
    module = cls.__module__

    if module == 'builtins':
        return cls.__qualname__

    return module + '.' + cls.__qualname__


def attrs_as_dict(obj):
    """
    Tries different methods to get a dict representation of obj.
    """

    attrs = {}

    if hasattr(obj, "__getstate__"):
        attrs = obj.__getstate__()
    elif hasattr(obj, "__dict__"):
        attrs = obj.__dict__
    elif isinstance(obj, dict):
        attrs = obj

    return attrs


def ext_setattr(obj, name, value):
    """
    Sets attributes objects and key-value pairs for dicts.
    """

    if type(obj) == dict:
        obj[name] = value
    else:
        setattr(obj, name, value)


class Serializer:
    """
    Converts an object tree into a json serializeable object tree.
    Large numpy arrays are automatically referenced and stored externally.
    """

    def __init__(self, path, hide_private=True):

        self.path = path
        self.hide_private = hide_private

        self.ext_path = os.path.join(path, "extern")

        os.makedirs(self.ext_path, exist_ok=True)

        self.saved_arrays = set()

    def serialize(self, obj, name=""):

        if self.hide_private and len(name) > 0 and name[0] == "_":
            return Skip

        # skip any kind of function
        if (isinstance(obj, types.FunctionType)
                or isinstance(obj, types.MethodType)
                or isinstance(obj, types.LambdaType)):
            return Skip

        # special treatment for large numpy arrays
        if type(obj) == np.ndarray:
            if obj.size > 100:
                byte_view = obj.view(np.uint8)
                path = hashlib.sha1(byte_view).hexdigest() + ".npy"
                np.save(os.path.join(self.ext_path, path), obj)
                self.saved_arrays.add(path)
                return {
                    "__class__": "__extern__",
                    "path": path
                }
            else:
                return {
                    "__class__": full_type(obj),
                    "dtype": obj.dtype.name,
                    "data": obj.tolist()
                }

        # already saved and memory-mapped numpy arrays
        if type(obj) == np.memmap:
            return {
                "__class__": "__extern__",
                "path": os.path.basename(obj.filename)
            }

        if type(obj) == list:
            return [self.serialize(v) for v in obj]

        attrs = attrs_as_dict(obj)

        if attrs == {}:
            # in case we don't find any serializeable attributes
            # we assume we have a primitive type and return that
            return obj

        ser_attrs = {}

        for k, v in attrs.items():
            val = self.serialize(v, k)
            if val != Skip:
                ser_attrs[k] = val

        # store the full type so we can compare it later
        ser_attrs["__class__"] = full_type(obj)

        return ser_attrs


class Loader:
    """
    Loads an object tree from a json file.
    External numpy arrays are automatically dereferenced and memory mapped.
    """

    def __init__(self, path):

        self.path = path
        self.ext_path = os.path.join(path, "extern")

    def load(self, obj, json_obj):

        attr_names = attrs_as_dict(obj)

        for k, v in attr_names.items():

            if k not in json_obj:
                continue

            t = type(v)

            jv = json_obj[k]
            jt = type(jv)

            # special care for numpy arrays
            if jt == dict and "__class__" in jv:

                cls = jv["__class__"]

                if cls == "__extern__":
                    jv = np.load(os.path.join(self.ext_path, jv["path"]),
                                 mmap_mode="r")
                    # we are lying about this one (actually np.memmap)
                    # in practice memmap should behave just like ndarray
                    jt = np.ndarray
                elif cls == "numpy.ndarray":
                    jv = np.array(jv["data"], dtype=jv["dtype"])
                    jt = np.ndarray

            if t == dict and jt == dict:
                # recursion ...
                self.load(v, jv)
            elif t == list and jt == list:
                objs = []
                for jjv in jv:
                    jjt = type(jjv)
                    if jjt == dict and "__class__" in jjv:
                        mod_name, cls_name = jjv["__class__"].rsplit(".", 1)
                        try:
                            mod = importlib.import_module(mod_name)
                            cls = getattr(mod, cls_name)
                            # assumes default initializeable type
                            objs.append(cls())
                        except Exception as e:
                            print("Loading list element error:", e)
                    else:
                        objs.append(jjv)
                ext_setattr(obj, k, objs)
            elif t == jt:
                # this usually happens for primitive types
                ext_setattr(obj, k, jv)
            elif v is None:
                # we just use whatever is contained in json
                ext_setattr(obj, k, jv)
            else:
                # maybe just more recursion then?
                self.load(v, jv)


def save(obj, directory):
    """
    Stores obj under a given directory.
    The directory will be created if it not already exists.
    """

    # write to json

    os.makedirs(directory, exist_ok=True)

    ser = Serializer(directory)
    rep = ser.serialize(obj)

    state_path = os.path.join(directory, "state.json")

    with open(state_path, "w+") as fd:
        json.dump(rep, fd, indent=2)

    # remove unused external numpy arrays

    on_disk = set(os.listdir(ser.ext_path))
    unused = on_disk - ser.saved_arrays

    for f in unused:
        os.remove(os.path.join(ser.ext_path, f))


def load(obj, path):
    """
    Updates obj with data stored at the given path.
    """

    state_path = os.path.join(path, "state.json")

    if not os.path.exists(state_path):
        return

    with open(state_path, "r") as fd:
        json_state = json.load(fd)

    Loader(path).load(obj, json_state)
