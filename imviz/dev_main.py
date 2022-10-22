"""
Because the automatic code reloading cannot reload the __main__ module,
we provide a module here, which can launch and reload a given class.

This frees the library user from providing such a main module themselves.

For specific needs not covered by this simple implementation, you may use
this module as a reference for your own (development) main module.
"""

import argparse

# i like this
from pydoc import locate

from imviz.dev import loop


def main():

    parser = argparse.ArgumentParser(
            description="Launch a class with automatic code reloading")

    parser.add_argument(
            "class_name",
            type=str,
            help="the name of the class to instantiate")

    parser.add_argument(
            "func_name",
            type=str,
            help="the name of the method to call")

    args, unknown = parser.parse_known_args()

    cls = locate(args.class_name)

    if cls is None:
        print(f"Could not find class {args.class_name}")
        return

    loop(cls, args.func_name)


if __name__ == "__main__":
    main()
