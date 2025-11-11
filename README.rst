.. README.rst

networking
==========

C/C++ networking examples for educational purposes.

About
-----

A first exploration into computer networking, mostly focusing on use of TCP/IP
connections via native socket implementations. I first started out with some C
examples directly using operating system C APIs but have slowly built a
header-only C++ abstraction layer to ease cross-platform development since
`Windows Sockets`_ 2 does things a bit differently compared to the typical
POSIX sockets implementation.

The C programs and C support library is written in ANSI C and should be
C99-compatible while the C++ programs and the C++ support library are written
in C++17. Moving to C++20 is of interest but is not a high priority currently.

.. _Windows Sockets: https://learn.microsoft.com/en-us/windows/win32/winsock/
   windows-sockets-start-page-2

Contents
--------

TBD. Currently includes client-server pairs for simple message acknowledgment
and for message echoing.

Building from source
--------------------

CMake_ >=3.21 is required to build from source on all platforms.

.. _CMake: https://cmake.org/cmake/help/latest/

\*nix
~~~~~

Building is easy with the provided ``build.sh`` build script. For usage, type

.. code:: shell

   ./build.sh --help

To build release binaries for this project, simply use the command

.. code:: shell

   ./build.sh -c Release

Simply typing ``./build.sh`` will build unoptimized binaries with debug symbols.

By default, the ``pdnnet`` support library is built as a shared library,
requiring that ``PDNNET_DLL`` be defined during compilation. To explicitly
request that ``pdnnet`` be built as a static library, specify
``-DBUILD_SHARED_LIBS=OFF``. E.g. to build release binaries with ``pdnnet``
built as a static library, one can use the command

.. code:: shell

   ./build.sh -c Release -Ca -DBUILD_SHARED_LIBS=OFF

Windows
~~~~~~~

Building is easy with the provided ``build.bat`` build script. For usage, type

.. code:: shell

   build --help

To build release binaries for this project, simple use the command

.. code:: shell

   build -c Release

Simply typing ``build`` will build unoptimized binaries and the program
database with debugging info. You can specify the target architecture using
the ``-a`` flag, e.g. to build 64-bit release binaries instead of the default
32-bit ones, use

.. code:: shell

   build -a x64 -c Release

Currently, the Visual Studio toolset used will be whichever is the default.

By default, the ``pdnnet`` support library is built as a shared library,
requiring that ``PDNNET_DLL`` be defined during compilation. To explicitly
request that ``pdnnet`` be built as a static library, specify
``-DBUILD_SHARED_LIBS=OFF``. E.g. to build 32-bit release binaries with
``pdnnet`` built as a static library, one can use the command

.. code:: shell

   build -c Release -Ca "-DBUILD_SHARED_LIBS=OFF"

The extra double quotes are needed to prevent the ``=`` from confusing CMD.
