#!/usr/bin/env python3
import sys
import os
import ctypes

if __name__ == '__main__':

    if len(sys.argv) < 2:
        print("Usage: test.py <JContainers DLL filepath>")
        sys.exit(1)

    argv = ('', '--gtest_filter=*.*')

    errno = -1
    try:
        lib = ctypes.cdll.LoadLibrary(sys.argv[1])
        cargs = (ctypes.c_char_p * len(argv))()
        for i in range(len(argv)):
            cargs[i] = argv[i].encode('utf-8')
        errno = lib.JC_runTests(len(argv), ctypes.byref(cargs))
        # Some leftovers
        os.rmdir ('path')
        os.rmdir ('path3')
    except BaseException as e:
        print("Error:", e)
    except:
        print("Unexpected error:", sys.exc_info()[0])
    sys.exit (errno)

