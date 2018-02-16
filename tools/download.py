#!/usr/bin/env python3
import sys
import urllib.request
import shutil

if __name__ == '__main__':
    if len (sys.argv) < 3:
        print ("Usage: download.py <URL> <destination>")
        exit (1)
    url = sys.argv[1]
    dst = sys.argv[2]
    with urllib.request.urlopen (url) as response, open (dst, 'wb') as out_file:
        shutil.copyfileobj (response, out_file)
