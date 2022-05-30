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
import shutil
import numbers

# i still like this
from pydoc import locate

import zarr
import numpy as np


def patch_zarr_indexing():
    """
    Evil, dark-magic, monkey-patching to make zarr array indexing
    behave even more like numpy indexing. 
    Will hopefully become unnecessary in future zarr versions.
    """

    original_getitem = zarr.Array.__getitem__
    original_setitem = zarr.Array.__setitem__

    def new_getitem(self, selection):

        try:
            val = original_getitem(self, selection)
        except IndexError:
            val = self.oindex[selection]

        return val

    def new_setitem(self, selection, values):

        try:
            original_setitem(self, selection, values)
        except IndexError:
            self.oindex[selection] = values

    zarr.Array.__getitem__ = new_getitem
    zarr.Array.__setitem__ = new_setitem


patch_zarr_indexing()


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


def ext_setattr(obj, name, value):
    """
    Sets attributes for objects and key-value pairs for dicts.
    """

    if type(obj) == dict:
        obj[name] = value
    else:
        setattr(obj, name, value)


ZARR_CHUNK_STORES = {}


def get_chunk_store(path):

    try:
        chunk_store = ZARR_CHUNK_STORES[path]
    except KeyError:
        chunk_store = zarr.LRUStoreCache(zarr.DirectoryStore(path), 10**9)
        ZARR_CHUNK_STORES[path] = chunk_store

    return chunk_store


class Serializer:
    """
    Converts an object tree into a json serializeable object tree.
    Large numpy arrays are automatically referenced and stored externally.
    """

    last_id = 0
    """
    Used to name external arrays. Will only be incremented.
    """

    def __init__(self, path, hide_private=True):

        self.path = path
        self.hide_private = hide_private

        self.ext_path = os.path.join(path, "extern")
        self.array_store = zarr.open(get_chunk_store(self.ext_path))

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
            if obj.size > 25:

                Serializer.last_id += 1

                path = str(Serializer.last_id)
                obj = self.array_store.array(path, obj)

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

        # already saved arrays
        if type(obj) == zarr.core.Array:
            self.saved_arrays.add(obj.path)
            return {
                "__class__": "__extern__",
                "path": obj.path
            }

        if type(obj) == list or type(obj) == tuple:
            jvs = []
            for i, v in enumerate(obj):
                jv = self.serialize(v, i, parent=obj)
                if jv is not Skip:
                    jvs.append(jv)
            return jvs

        # try to get a dict representation of the object

        attrs = None

        if hasattr(obj, "__getstate__"):
            attrs = obj.__getstate__()
        elif hasattr(obj, "__dict__"):
            attrs = obj.__dict__
        elif hasattr(obj, "__slots__"):
            attrs = {n: getattr(obj, n) for n in obj.__slots__}
        elif isinstance(obj, dict):
            attrs = obj

        if attrs is None:
            # in case we don't find any serializeable attributes
            # we check if we have a primitive type and return that
            if isinstance(obj, (numbers.Integral, numbers.Real, str, type(None))):
                return obj
            else:
                print(f"Warning: cannot save object {key} of type {full_type(obj)}")
                return Skip

        ser_attrs = {}

        for k, v in attrs.items():
            val = self.serialize(v, k, parent=obj)
            if val is not Skip:
                ser_attrs[k] = val

        if len(ser_attrs) == 0:
            return Skip

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

        self.array_store = zarr.open(get_chunk_store(self.ext_path))

        self.loaded_arrays = set()

    def load(self, obj, json_obj):

        t = type(obj)
        jt = type(json_obj)

        # before we do anything else we check if we
        # can convert the json obj to a numpy array

        if jt == dict and "__class__" in json_obj:

            cls = json_obj["__class__"]

            if cls == "__extern__":
                path = json_obj["path"]
                json_obj = self.array_store[path]
                self.loaded_arrays.add(path)
                # we are lying about this one (actually zarr.core.Array)
                # in practice it should behave (mostly) like ndarray
                jt = np.ndarray
            elif cls == "numpy.ndarray":
                json_obj = np.array(json_obj["data"], dtype=json_obj["dtype"])
                jt = np.ndarray

        # try to get a dict representation of the object

        attrs = None

        if hasattr(obj, "__dict__"):
            attrs = obj.__dict__
        elif hasattr(obj, "__slots__"):
            attrs = {n: getattr(obj, n) for n in obj.__slots__}
        elif isinstance(obj, dict):
            attrs = obj

        # now check how we should continue

        if attrs is not None and jt == dict:

            # handles general objects and dicts

            if hasattr(obj, "__setstate__"):
                ld = {k: self.load(None, v) for k, v in json_obj.items()}
                if "__class__" in ld:
                    del ld["__class__"]
                obj.__setstate__(ld)
            elif isinstance(obj, dict) and len(obj) == 0:
                # if obj is dict-like and empty,
                # we accept whatever is stored in the json file
                ld = {k: self.load(None, v) for k, v in json_obj.items()}
                if "__class__" in ld:
                    del ld["__class__"]
                obj.update(ld)
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
                    if jo is not Skip:
                        jos.append(jo)
                else:
                    jos.append(obj[i])
            return t(jos)
        elif obj is None:
            # we have nothing to match
            if jt == dict and "__class__" in json_obj:
                # try constructing a new object
                cls_name = json_obj["__class__"]
                try:
                    cls = locate(cls_name)
                    # assumes default initializeable type
                    return self.load(cls(), json_obj)
                except Exception:
                    print(f"Warning: skipping init of unknown type {cls_name}")
                    return Skip
            else:
                # just use whatever is contained in json
                return json_obj
        elif attrs is None:
            # this usually happens for primitive types
            if t == jt:
                return json_obj
            else:
                # if the types do no match,
                # there is still a chance we can cast
                try:
                    return t(json_obj)
                except Exception:
                    pass
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

    rep["__imviz_last_id"] = Serializer.last_id

    state_path = os.path.join(directory, "state.json")

    with open(state_path, "w+") as fd:
        json.dump(rep, fd, indent=2)

    # remove unused external numpy arrays

    on_disk = set(ser.array_store.keys())
    unused = on_disk - ser.saved_arrays

    for k in unused:
        del ser.array_store[k]

        # because zarr does not clean up emtpy folders
        arr_path = os.path.join(ser.ext_path, k)
        if os.path.isdir(arr_path):
            shutil.rmtree(arr_path)


def load(obj, path):
    """
    Updates obj with data stored at the given path.
    """

    print("starting loading")

    state_path = os.path.join(path, "state.json")

    if not os.path.exists(state_path):
        return

    with open(state_path, "r") as fd:
        json_state = json.load(fd)

    Serializer.last_id = json_state["__imviz_last_id"]

    lod = Loader(path)
    lod.load(obj, json_state)

    # remove unused external numpy arrays

    on_disk = set(lod.array_store.keys())
    unused = on_disk - lod.loaded_arrays

    for k in unused:
        del lod.array_store[k]

        # because zarr does not clean up emtpy folders
        arr_path = os.path.join(lod.ext_path, k)
        if os.path.isdir(arr_path):
            shutil.rmtree(arr_path)
