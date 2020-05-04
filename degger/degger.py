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


def find_typerefs(node, typename):
    if node.kind.is_reference():
        if node.spelling == "class std::" + typename:
            print("Found %s [Line=%s, Col=%s]"%(typename, node.location.line, node.location.column))
    for c in node.get_children():
        find_typerefs(c, typename)


def getParentFunctionDeclIfAny(cursor):
    if cursor.semantic_parent.kind == None:
        return None
    elif cursor.semantic_parent.kind == clang.cindex.CursorKind.FUNCTION_DECL:
        return cursor.semantic_parent
    else:
        getParentFunctionDecl(cursor.semantic_parent)


def getReqIDIfAny(raw_comment):
    reqIDRegEx = "\s([1-9](\.[1-9])*?)\s"
    return re.findall(reqIDRegEx, raw_comment)


def generateOutput(traceability_list, argparser):
    if argparser.args.outtype == "csv":
        fs = argparser.args.fs
        for elem in traceability_list:
            dump = str()
            for subelem in elem:
                dump += subelem + fs
            print(dump)

def getListAsString(traceability_list, argparser):
    fs = argparser.args.fs
    dump = str()
    for elem in traceability_list:
        for subelem in elem:
            dump += subelem + fs
        dump += "\n"
    return dump


def getcompbase(source_file_path, compbase_path):
    compdb = clang.cindex.CompilationDatabase.fromDirectory(compbase_path)
    compcomms = compdb.getCompileCommands(source_file_path)
    for comm in compcomms:
        print(comm.arguments)
        print(comm.directory)
        print(comm.filename)
    return compcomms


# write code here
def premain(argparser):
    signal.signal(signal.SIGINT, SigHandler_SIGINT)
    #here
    traceability_list = []
    traceability_list.append(["Requirement ID", "Function name", "Function line", "Function column",
        "Function file", "comment line", "comment column", "comment file"])
    if argparser.args.file is not None:
        for src_file in argparser.args.file:
            compile_command = getcompbase(src_file, argparser.args.compdb)
            index = clang.cindex.Index.create()
            tu = index.parse(src_file)
            for token in tu.cursor.get_tokens():
                if token.kind == clang.cindex.TokenKind.COMMENT:
                    if token.spelling.find("@trace") != -1:
                        src_range = token.extent
                        cursor = token.cursor
                        declFunc = getParentFunctionDeclIfAny(token.cursor)
                        if declFunc.spelling != None:
                            req_id = token.spelling.strip()
                            req_id = req_id.strip("/")
                            req_id = req_id.strip("*")
                            req_id = req_id[7:]
                            req_id = req_id.strip()
                            traceability_list.append([req_id, declFunc.spelling, repr(declFunc.location.line),
                                repr(declFunc.location.column), repr(declFunc.location.file), repr(token.location.line),
                                repr(token.location.column), repr(token.location.file)])
    else:
        print("you have to specify the name of the file...")

    if len(traceability_list) > 0:
        generateOutput(traceability_list, argparser)
        out_file = open(src_file+".trace.txt", "w")
        out_file.write(getListAsString(traceability_list, argparser))


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
