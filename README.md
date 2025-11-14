[![Build status](https://ci.appveyor.com/api/projects/status/r8kex3xnxmktn1sq?svg=true)](https://ci.appveyor.com/project/ryobg/jcontainers)
[![Latest release](https://img.shields.io/github/release/ryobg/jcontainers/all.svg)](https://github.com/ryobg/jcontainers/releases)

<img src="logo.png?raw=true" height="256">

# JContainers (64-bit)

A project to extend [Skyrim's Papyrus
scripting](https://www.creationkit.com/index.php?title=Category:Papyrus) with
[JSON formatted](https://json.org/) serializable data structures.

> **Important**
>
> This project is fork of the original [JContainers](https://github.com/SilverIce/JContainers). It
> strives to convert and mash it up to the new Skyrim Special 64-bit edition. Cudos to the original
> author and all of his supporters!

### Why?

If you are programmer, sooner or later you'll notice the lack of many useful features in Papyrus.
There is no way to:

- Append or erase values from arrays
- Put an array into an array (i.e. no nested arrays)
- Put multiple value types into a single array
- Have associative containers 
- Be able to load or save a data into a file

### Solution

Since there is no source code of the Papyrus virtual machine, it is tricky and no easy to extend the
existing Papyrus Array type or add new data structure types. JContainers implements from scratch its
own data structures, garbage collector and other infernals. Features offered:

- Data structures: arrays and associative containers (a.k.a. maps or dictionaries)
- Import and export data to and from JSON files
- Embedded, lightweight scripting with [Lua](https://www.lua.org/)
- Interaction with JContainers via C++ interface.

### Full documentation

[Latest documentation](https://github.com/ryobg/jcontainers/wiki)

### Can I help?

Sure! Feel free to do whatever you think is good - post feature requests, report bugs, improve Wiki
or source code.

# Building from source

### Prerequisites

* [Microsoft Visual Studio 2019](https://www.visualstudio.com/downloads/) 
  Community Edition with Visual C++ support would suffice. All project files are of that version,
  but with a bit of manual work they may be converted to older versions too (e.g. 2013, though
  issues most certainly will arrise).
* A [Python](https://www.python.org/downloads/) environment for Windows, version 3.4 or later.
  This is needed to run some helper scripts for testing, building distributions and any other small
  helpful tasks. Its `python.exe` should be available on the PATH variable.
* The GIT revisioning system and/or GitHub account may help if you want to contribute or work more
  easily with the public repository of this project.

### First time setup

1. Run `git submodule update --init --recursive` so that all dependencies like Jannson, Google Test
   and etc. get downloaded and linked to the correct revisions.
2. Run the JContainer's `tools\build_boost.bat` file. It should manage to download, unpack,
   bootstrap and build the neccessary libraries from Boost (version 1.67 currently). If you encounter
   boost linking errors later on, you can attempt to specify the msvc version when running the batch
   file, e.g. `tools\build_boost.bat vc141`.
3. Run also the `tools\merge_skse.bat` file. It should extract the stripped down and bundled SKSE64
   and SKSE VR distributions into the local source tree.
4. Open the `JContainers.sln` file with Visual Studio and Rebuild the whole solution. It will take
   some time.
5. After successfull build, run from the command line `python tools\install.py x64\Release 64`.
   Eventually swap `Release` for `Debug` - depending on what kind of distribution was build and
   actually is wanted in the `dist\` folder. The last argument, `64` could be also `VR`, but then
   the configuration should be either `ReleaseVR` or `DebugVR`.
6. Optionaly, run `python tools\test.py x64\Release\Data\SKSE\Plugins\JContainers64.dll`. Again it
   depends whether `Release` or `Debug` (or `ReleaseVR` and `DebugVR`) builds should be tested. Note
   however that step 4, must be ran first!

That's it!

