#!/usr/bin/env python3
import os
import sys
import shutil

ROOT = os.path.abspath (os.path.join (os.path.dirname(__file__), '..'))
DIST_DIR = os.path.join (ROOT, 'dist')

#------------------------------------------------------------------------------

def makepath (*args):
    """ Create recursively any directory path composed from the joined args of this function. """
    path = os.path.join (*args)
    if not os.path.exists (path):
        print("Make dir:", os.path.relpath (path, ROOT))
        os.makedirs (path)
    return path

#------------------------------------------------------------------------------

class JCLib(object):

    suffix = ''
    name = ''

    def __init__(self, location):
        import ctypes

        print ('Load:', os.path.relpath (location, ROOT))
        self.location = location
        self.lib = ctypes.CDLL (location);
        self.lib.JC_versionString.restype = ctypes.c_char_p
        self.lib.JC_produceCode.argtypes = [ctypes.c_char_p]

    def produce_code (self, into):
        makepath (into)
        print ('Produce scripts:', os.path.relpath (into, ROOT))
        self.lib.JC_produceCode (into.encode ('utf-8'))

    def version (self):
        return self.lib.JC_versionString().decode('utf-8')

#------------------------------------------------------------------------------

class Config(object):
    def __init__(self, path):
        self.path = path
        self._jcLib = None

    @property
    def origin(self):
        return os.path.join(ROOT, self.path)

    @property
    def pscDir(self):
        return os.path.join(self.origin, 'Data', 'scripts', 'source')

    @property
    def compiledDir(self):
        return os.path.join(self.origin, 'Data', 'scripts')

    @property
    def dataDir(self):
        return os.path.join(self.origin, 'Data')

    @property
    def pluginDir(self):
        return os.path.join(self.origin, 'Data', 'SKSE', 'Plugins')

    @property
    def jcLibPath(self):
        return os.path.join(self.pluginDir, JCLib.name)

    @property
    def jcLib(self):
        if not self._jcLib:
            self._jcLib = JCLib(self.jcLibPath)

        return self._jcLib

#------------------------------------------------------------------------------

def try_system (command):
    if os.system(command) != 0:
        raise Exception(command, 'has failed')

#------------------------------------------------------------------------------

def compile_scripts (src, dst):
    def quotes(s):
        return '"' + s + '"'

    cdir = os.path.join (ROOT, 'tools', 'PapyrusCompiler')
    args = [
        os.path.join (cdir, 'PapyrusCompiler.exe'),
        quotes (src),
        '-all',
        '-quiet',
        '-f=' + quotes (os.path.join (cdir, 'TESV_Papyrus_Flags.flg')),
        '-i=' + ';'.join (map (quotes, [src, os.path.join (ROOT, 'tools', 'PapyrusScripts')])),
        '-o=' + quotes (dst)
    ]

    print ('Compile scripts:', os.path.relpath (dst, ROOT))
    try_system (' '.join (args))

#------------------------------------------------------------------------------

def make_archive (dst, src):
    print ('Make archive:', os.path.relpath (dst, ROOT))
    exe = os.path.join (ROOT, 'tools', '7za.exe')
    try_system ('{} a -mx9 "{}.7z" "{}" > nul'.format (exe, dst, src))

#------------------------------------------------------------------------------

if __name__ == '__main__':
    if len (sys.argv) < 3:
        print ("Usage: install.py x64/[Release[VR]|Debug[VR]] [64|VR]")
        exit (1)

    JCLib.suffix = sys.argv[2]
    JCLib.name = "JContainers" + JCLib.suffix + ".dll"

    config = Config (sys.argv[1])

    print ("Setup Skyrim Mod tree...")
    srcdata = os.path.join (ROOT, 'src', 'Data')
    shutil.rmtree (config.dataDir, ignore_errors=True)
    shutil.copytree (srcdata, config.dataDir)
    shutil.copy2 (os.path.join (config.origin, JCLib.name), config.jcLibPath)

    print ("Generate and compile scripts...")
    config.jcLib.produce_code (config.pscDir)
    compile_scripts (config.pscDir, config.compiledDir)

    print ("Recreate distros...")
    shutil.rmtree (DIST_DIR, ignore_errors = True)

    dst = os.path.join (DIST_DIR, 'Data')
    shutil.copytree (config.dataDir, dst, \
            ignore = shutil.ignore_patterns ('*.exp', '*.lib', 'test_data'))
    make_archive (os.path.join (DIST_DIR, \
            'JContainers' + JCLib.suffix + '-v' + config.jcLib.version ()), dst)
    shutil.rmtree (dst, ignore_errors = True)

    print ("API example skipped - its under revision...")
    print ("All done.")

#------------------------------------------------------------------------------

