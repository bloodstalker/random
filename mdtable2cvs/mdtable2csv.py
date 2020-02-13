#!/usr/bin/python3
# _*_ coding=utf-8 _*_

import argparse
import code
import readline
import signal
import sys
import mistune
import subprocess
try:
    assert mistune.__version__ >= '2.0.0'
except AssertionError:
    print("mistune version is way too old for this to work. get mistune v2.0.0 or newer.")
    print("yours is version " + mistune.__version__ + ".")
    exit(1)


def SigHandler_SIGINT(signum, frame):
    print()
    sys.exit(0)


class Argparser(object):
    def __init__(self):
        parser = argparse.ArgumentParser()
        parser.add_argument("--md", type=str, nargs="+", help="comma-separated list of path(s) to the markdown file(s)")
        parser.add_argument("--bool", action="store_true", help="bool", default=False)
        parser.add_argument("--dbg", action="store_true", help="debug", default=False)
        self.args = parser.parse_args()


# write code here
def misty(argparser):
    signal.signal(signal.SIGINT, SigHandler_SIGINT)
    #here
    if argparser.args.md is not None:
        req_table_txt = None
        for md in argparser.args.md:
            mdd = open(md)
            mdd_str = str()
            for line in mdd:
                mdd_str += line

            markdown = mistune.markdown(mdd_str, renderer='ast')

            for elem in markdown:
                try:
                    for child in elem["children"]:
                        if child["type"]:
                            if child["text"].find("Req id") != -1:
                                req_table_txt = child["text"]
                except:
                    pass
            if req_table_txt is not None:
                csv = open(md+".csv", "w")
                csv.write(req_table_txt)
                csv.write("\n")
    else:
        print("need to type in the name of the markdown file(s)...")


def main():
    argparser = Argparser()
    if argparser.args.dbg:
        try:
            misty(argparser)
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
        misty(argparser)


if __name__ == "__main__":
    main()
