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

# i still like this
from pydoc import locate

import numpy as np

from imviz.common import bundle


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

    attrs = None

    if hasattr(obj, "__getstate__"):
        attrs = obj.__getstate__()
    elif hasattr(obj, "__dict__"):
        attrs = obj.__dict__
    elif isinstance(obj, dict):
        attrs = obj

    return attrs


def ext_setattr(obj, name, value):
    """
    Sets attributes for objects and key-value pairs for dicts.
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

    def serialize(self, obj, key="", parent=None):

        if type(key) == str:
            if self.hide_private and len(key) > 0 and key[0] == "_":
                return Skip

        # skip any kind of function
        if (isinstance(obj, types.FunctionType)
                or isinstance(obj, types.MethodType)
                or isinstance(obj, types.LambdaType)):
            return Skip

        # special treatment for numpy arrays
        if type(obj) == np.ndarray:
            if obj.size > 100:
                byte_view = obj.view(np.uint8)

                path = hashlib.sha1(byte_view).hexdigest() + ".npy"
                file_path = os.path.join(self.ext_path, path)

                np.save(file_path, obj)
                obj = np.load(file_path, mmap_mode="r")

                if type(key) == str:
                    ext_setattr(parent, key, obj)
                elif type(key) == int:
                    parent[key] = obj

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

        if type(obj) == list or type(obj) == tuple:
            jvs = []
            for i, v in enumerate(obj):
                jv = self.serialize(v, i, parent=obj)
                if jv != Skip:
                    jvs.append(jv)
            return jvs

        attrs = attrs_as_dict(obj)

        if attrs is None:
            # in case we don't find any serializeable attributes
            # we assume we have a primitive type and return that
            return obj

        ser_attrs = {}

        for k, v in attrs.items():
            val = self.serialize(v, k, parent=obj)
            if val != Skip:
                ser_attrs[k] = val

        # store the full type so we can compare it later
        ser_attrs["__class__"] = full_type(obj)

        return ser_attrs


class Loader:
    """
    Loads an object tree from a json file.
    External numpy arrays are automatically dereferenced and mem-mapped.
    """

    def __init__(self, path):

        self.path = path
        self.ext_path = os.path.join(path, "extern")

    def load(self, obj, json_obj):

        t = type(obj)
        jt = type(json_obj)

        # before we do anything else we check if we
        # can convert the json obj to a numpy array

        if jt == dict and "__class__" in json_obj:

            cls = json_obj["__class__"]

            if cls == "__extern__":
                json_obj = np.load(os.path.join(
                                 self.ext_path,
                                 json_obj["path"]),
                             mmap_mode="r")
                # we are lying about this one (actually np.memmap)
                # in practice memmap should behave just like ndarray
                jt = np.ndarray
            elif cls == "numpy.ndarray":
                json_obj = np.array(json_obj["data"], dtype=json_obj["dtype"])
                jt = np.ndarray

        attrs = attrs_as_dict(obj)

        # now check how we should continue

        if attrs is not None and jt == dict:
            # handles general objects and dicts
            if hasattr(obj, "__setstate__"):
                ld = {k: self.load(None, v) for k, v in json_obj.items()}
                if "__class__" in ld:
                    del ld["__class__"]
                obj.__setstate__(ld)
            else:
                for k, v in attrs.items():
                    if k in json_obj:
                        ext_setattr(obj, k, self.load(v, json_obj[k]))
            return obj
        elif (t == list or t == tuple) and jt == list:
            # handles lists and tuples
            jos = []
            for i in range(max(len(obj), len(json_obj))):
                if i < len(json_obj):
                    jv = json_obj[i]
                    if i < len(obj):
                        # load into existing object
                        jo = self.load(obj[i], jv)
                    else:
                        # otherwise create new object
                        jo = self.load(None, jv)
                    if jo != Skip:
                        jos.append(jo)
                else:
                    jos.append(obj[i])
            return t(jos)
        elif t == jt:
            # this usually happens for primitive types
            return json_obj
        elif obj is None:
            # we have nothing to match
            if jt == dict and "__class__" in json_obj:
                # try constructing a new object
                try:
                    cls = locate(json_obj["__class__"])
                    # assumes default initializeable type
                    return self.load(cls(), json_obj)
                except Exception:
                    print("Warning: skipping object of unknown type \""
                          + cls_name + "\"")
                    return Skip
            else:
                # just use whatever is contained in json
                return json_obj
        else:
            # nothing more we can do
            return obj


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


class AltSubTest:

    def __init__(self):

        self.aa = [1, 2, 3]
        self.bb = (False, True, 1)


class Test:

    def __init__(self):

        self.a = 1
        self.b = 2.0
        self.c = "hello"
        self.d = [1, 2, 3]
        self.e = (1, 2, 3)
        self.f = False
        self.g = True
        self.h = np.zeros((210,))

        self.i = AltSubTest()
        self.j = [AltSubTest(), AltSubTest(), AltSubTest()]

        self.k = {
                "ka": AltSubTest(),
                "kb": False
            }

        self.m = [bundle(a=4, b=4, c=4), bundle(a=4, b=4, c=4)]
        self.n = (bundle(a=4, b=4, c=4), False)

        self.fail = np.float32


def main():

    test = Test()
    save(test, "./test_save")

    print(type(test.h))

    load(test, "./test_save")

    print(test.__dict__)
    print(test.i.__dict__)
    print(test.k)
    print(test.k["ka"].__dict__)
    print(test.m)
    print(test.n)


if __name__ == "__main__":
    main()
