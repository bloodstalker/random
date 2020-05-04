#!/usr/bin/env python3
# _*_ coding=utf-8 _*_

import argparse
import code
import readline
import signal
import sys
import subprocess
import re
import os

if sys.version_info < (3, 7):
    shell_result = subprocess.run(["llvm-config", "--src-root"], stdout=subprocess.PIPE)
else:
    shell_result = subprocess.run(["llvm-config", "--src-root"], capture_output=True)

#@FIXME-for some reason cygwin doesnt like to unpack the source files so we have to use this trashy way to import the python bindings.
if os.uname().sysname.find("CYGWIN") != -1:
    sys.path.insert(0, "/cygdrive/d/LLVM/llvm-8.0.1.src/bindings/python")
    sys.path.insert(0, "/cygdrive/d/LLVM/cfe-8.0.1.src/bindings/python")
else:
    sys.path.insert(0, shell_result.stdout.decode("utf-8")[:-1] + "/bindings/python")
    sys.path.insert(0, shell_result.stdout.decode("utf-8")[:-1] + "/tools/clang/bindings/python")

import clang.cindex
import llvm
if os.uname().sysname.find("CYGWIN") != -1:
    clang.cindex.Config.set_library_file("/cygdrive/c/Program Files/LLVM8/bin/libclang.dll")
else:
    clang.cindex.Config.set_library_file("/home/bloodstalker/extra/llvm-clang-4/build/lib/libclang.so")
from clang import enumerations as clangenums


def SigHandler_SIGINT(signum, frame):
    print()
    sys.exit(0)


class Argparser(object):
    def __init__(self):
        parser = argparse.ArgumentParser()
        parser.add_argument("--file", nargs="+", type=str, help="comma-separated list of files.")
        parser.add_argument("--compdb", type=str, help="path to compilation database directory, not the file itself.")
        parser.add_argument("--outtype", type=str, default="csv", help="the output type. default is \"csv\".")
        parser.add_argument("--fs", type=str, default="|", help="only meaningful when outtype is set to csv. default is \"|\".")
        parser.add_argument("--dbg", action="store_true", help="debug", default=False)
        self.args = parser.parse_args()

def get_function_definitions(node):
    if node.kind.FUNCTION_DECL and node.is_definition():
        if "makejmptable" == node.spelling:
            #print(node.kind)
            print(node.extent)
            for arg in node.get_arguments():
                if arg.type.kind == clang.cindex.TypeKind.POINTER:
                    #print(arg.spelling)
                    #print(arg.type.spelling)
                    print("\\valid("+arg.spelling+")")
            for c_node in node.get_children():
                print(c_node.spelling)
    for c_node in node.get_children():
        get_function_definitions(c_node)


# write code here
def premain(argparser):
    signal.signal(signal.SIGINT, SigHandler_SIGINT)
    #here
    if argparser.args.file is not None:
        for src_file in argparser.args.file:
            index = clang.cindex.Index.create()
            tu = index.parse(src_file)
            get_function_definitions(tu.cursor)


def main():
    argparser = Argparser()
    if argparser.args.dbg:
        try:
            premain(argparser)
        except Exception as e:
            if hasattr(e, "__doc__"):
                print(e.__doc__)
            if hasattr(e, "message"):
                print(e.message)
            variables = globals().copy()
            variables.update(locals())
            shell = code.InteractiveConsole(variables)
            shell.interact(banner="DEBUG REPL")
    else:
        premain(argparser)


if __name__ == "__main__":
    main()
