                  Total Phase Aardvark Sample Code
                  --------------------------------

Introduction
------------
This directory contains examples that use the Aardvark Rosetta C/C++
language bindings.


Contents
--------
- Makefile for Windows GCC-MinGW32, Linux, and Darwin.
- Visual C++ workspace for MSVC++ 6.0 and above

See top level EXAMPLES.txt for descriptions of each example.


Build Instructions
------------------
In each case, PROGRAM can be any of the sample programs, without the
'.c' extension.

Windows Visual C++ 6.0:
1) Open MSVC/Makefile.dsw
2) Build | Batch Build... | Build
3) Optionally place aardvark.dll in the PATH
4) _output/PROGRAM

Windows Visual C++ 2008:
1) Open MSVC2008/Makefile.sln
2) Build | Batch Build... | Build
3) Optionally place aardvark.dll in the PATH
4) _output/PROGRAM

Windows gcc-mingw32:
1) Install GCC MinGW32.
   The latest version can be downloaded from the MinGW website:
   http://www.mingw.org/
2) Install MSYS
   The latest version can be downloaded from the MinGW website:
   http://www.mingw.org/
3) Type 'make' at the MSYS command line
4) _output/PROGRAM

Linux and Darwin:
1) Type 'make' at the command line
2) _output/PROGRAM


