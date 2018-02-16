[![Build status](https://ci.appveyor.com/api/projects/status/r8kex3xnxmktn1sq?svg=true)](https://ci.appveyor.com/project/ryobg/jcontainers)

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
existing Papyrus Array type or add new data structure type. Thus JContainers implements it's own
data structures, garbage collector and other infernal stuff from scratch. Features offered:

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

* [Microsoft Visual Studio 2017](https://www.visualstudio.com/downloads/) 
  Community Edition would suffice, its free. You will need its Visual C++ component at least.
  All the project files are converted to that version, it with a bit of manual work it may work with
  older versions too (e.g. 2013) but issues may arrise. Later versions are more compatible.
* A [Python](https://www.python.org/downloads/) environment for Windows, version 3.4 or later.
  This is needed to run some helper scripts for testing, building distributions and any other small
  helpful tasks. Its executable should be available on the PATH variable.
* The GIT revisioning system and/or GitHub account may help if you want to contribute or work more
  easily with the public repository of this project.

### First time setup

1. Run from the JContainer's tools folder the `build_boost.bat` file. It should manage to download,
   unpack, bootstrap and build the neccessary libraries from Boost (version 1.66 currently).
2. Open the `JContainers.sln` file with Visual Studio and Rebuild the whole solution. It will take
   some time.
3. After successfull build, run from the command line `python tools\install.py x64\Release`.
   Eventually swap `Release` for `Debug` - depending on what kind of distribution was build and
   actually is wanted in the `dist` folder.
4. Optionaly, run `python tools\test.py x64\Release\Data\SKSE\Plugins\JContainers64.dll`. Again it
   depends whether `Release` or `Debug` builds should be tested. Note however that step 3, must be
   ran first!

That's it!

