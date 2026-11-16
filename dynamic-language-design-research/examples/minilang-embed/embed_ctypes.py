#!/usr/bin/env python3
"""embed_ctypes.py: the same C API from Python, through ctypes (standard library only).

Python is the host here: it loads minilang as a shared library, registers a
Python function as a host function, and calls minilang, which calls back into
Python. Build the library first:
    gcc -std=c11 -Wall -Wextra -Werror -O2 -shared -o minilang.dll minilang.c -lm          (Windows)
    gcc -std=c11 -Wall -Wextra -Werror -O2 -shared -fPIC -o libminilang.so minilang.c -lm  (Linux)
Then:
    python3 embed_ctypes.py
"""
import ctypes
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "minilang-vm"))
import minilang_vm  # noqa: E402  the stage 4 compiler, to make the bytecode file
import minilang  # noqa: E402


class Value(ctypes.Structure):  # ml_value: a handle
    _fields_ = [("id", ctypes.c_uint32), ("gen", ctypes.c_uint32)]


ML_OK, ML_EXCEPTION = 0, 1
HOSTFN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(Value),
                          ctypes.POINTER(Value), ctypes.c_void_p)


def load_library():
    name = "minilang.dll" if os.name == "nt" else "libminilang.so"
    lib = ctypes.CDLL(os.path.join(HERE, name))
    vm, val, pval = ctypes.c_void_p, Value, ctypes.POINTER(Value)
    for fn, args, res in (
        ("ml_open", [ctypes.c_void_p], vm), ("ml_close", [vm], ctypes.c_size_t),
        ("ml_load", [vm, ctypes.c_char_p, ctypes.c_size_t, pval], ctypes.c_int),
        ("ml_run", [vm, val, pval], ctypes.c_int), ("ml_get_global", [vm, ctypes.c_char_p, pval], ctypes.c_int),
        ("ml_call", [vm, val, ctypes.c_int, pval, pval], ctypes.c_int),
        ("ml_register", [vm, ctypes.c_char_p, HOSTFN, ctypes.c_int, ctypes.c_void_p], ctypes.c_int),
        ("ml_new_string", [vm, ctypes.c_char_p, ctypes.c_size_t, pval], ctypes.c_int),
        ("ml_get_string", [vm, val, ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_size_t)], ctypes.c_int),
        ("ml_throw", [vm, val], ctypes.c_int), ("ml_error_message", [vm], ctypes.c_char_p),
    ):
        getattr(lib, fn).argtypes, getattr(lib, fn).restype = args, res
    return lib


def main():
    lib = load_library()
    vm = lib.ml_open(None)

    def new_string(text):
        data = text.encode("utf-8")
        out = Value()
        lib.ml_new_string(vm, data, len(data), ctypes.byref(out))
        return out

    def get_string(handle):
        s, n = ctypes.c_char_p(), ctypes.c_size_t()
        if lib.ml_get_string(vm, handle, ctypes.byref(s), ctypes.byref(n)) != ML_OK:
            return None
        return ctypes.string_at(s, n.value).decode("utf-8")

    @HOSTFN
    def py_upper(_vm, nargs, args, result, _ud):  # a host function written in Python
        text = get_string(args[0])
        if text is None:
            return lib.ml_throw(vm, new_string("py_upper() expects a string"))  # caught by minilang
        result[0] = new_string(text.upper())
        return ML_OK

    lib.ml_register(vm, b"py_upper", py_upper, 1, None)
    with open(os.path.join(HERE, "programs", "shout.ml"), encoding="utf-8") as f:
        code = minilang_vm.dump_program(minilang_vm.compile_program(minilang.parse(f.read())))
    program, ignored = Value(), Value()
    if lib.ml_load(vm, code, len(code), ctypes.byref(program)) or lib.ml_run(vm, program, ctypes.byref(ignored)):
        sys.exit("cannot run shout.ml: %s" % lib.ml_error_message(vm).decode())
    def global_value(name):
        out = Value()
        lib.ml_get_global(vm, name, ctypes.byref(out))
        return out

    def call(name, arg):
        out = Value()
        status = lib.ml_call(vm, global_value(name), 1, ctypes.byref(arg), ctypes.byref(out))
        if status == ML_OK:
            print("%s -> %s" % (name.decode(), get_string(out)))
        else:
            print("%s -> status %d: %s" % (name.decode(), status, lib.ml_error_message(vm).decode()))

    call(b"shout", new_string("hello, world"))  # minilang -> Python, back to minilang
    call(b"safe_shout", new_string("ctypes"))
    call(b"safe_shout", global_value(b"len"))   # not a string: py_upper() throws, minilang catches it
    call(b"shout", global_value(b"len"))        # nothing catches it: ML_EXCEPTION reaches Python
    left = lib.ml_close(vm)
    return 0 if left == 0 else 3


if __name__ == "__main__":
    sys.exit(main())
