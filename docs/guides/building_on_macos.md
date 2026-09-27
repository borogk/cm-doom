# Building cm-doom on macOS

This is a basic guide for making a release build of cm-doom on macOS using tools and libraries from brew.

## Prerequisites

In order to build cm-doom, the following tools are needed:
- Xcode's Command Line Tools, installed by running `xcode-select --install` from a terminal.
- The Homebrew package manager, refer to [this page](https://brew.sh/) for installation.

This guide assumes all the commands are ran from the root directory of the repository, make sure to move into the
directory after cloning the sources:

```
git clone https://github.com/borogk/cm-doom.git
cd cm-doom
```

## Installing Dependencies

All the tools and library dependencies can be installed in a single command:

```
brew bundle
```

## Building

cm-doom is built using CMake. The project first needs to be configured:

```
cmake -S prboom2 -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

If successful, the project can be built using the following command:

```
cmake --build build
```

## Installing

cm-doom can be installed system-wide by running the following command:
```
cmake --install build
```

The default installation location is `/usr/local`, this can be changed by configuring the project with
`--install-prefix /custom/install/prefix`

## Packaging

CPack is used to create relocatable packages that do not depend on having all the dependencies installed system-wide.
The package can be generated **from the build directory** with the following command:

```
cd build
cpack -G External
```

This will generate a file called `cm-doom-x.y.z-Darwin.zip` (where `x.y.z` corresponds to the current version).
