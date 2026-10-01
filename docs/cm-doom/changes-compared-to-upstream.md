# Changes compared to upstream

Below is the exhaustive list of all changes to `cm-doom` fork compared to [DSDA-Doom](https://github.com/kraflab/dsda-doom).

> [!TIP]
> This document also serves as a checklist of things to look out for after merging from upstream

## Cameraman features

Pretty much all Cameraman-specific features are put into the `cman.c`/`cman.h` module.
Any interactions between this module and the existing codebase are kept to a bare minimum.
This approach should reduce any risk of messing up existing functionality.

### Main module

[cman.c](../../prboom2/src/cman.c),
[cman.h](../../prboom2/src/cman.h)
- These contain the bulk of cm-doom logic, are specific to this port and should never cause merge conflicts

### Initialization

[d_main.c](../../prboom2/src/d_main.c)
- `cman.h` should be included
- `DoLooseFiles()` function should allow `.cman` file extension be omitted when paired with `-cman` argument
- `D_DoomMainSetup()` function should call `CMAN_Init()`, follow code comments to decide on the exact placement

### Game loop integration

[g_game.c](../../prboom2/src/g_game.c)
- `cman.h` should be included
- `P_WalkTicker()` function should call `CMAN_Ticker()` as the very first thing and abort futher processing depending on its result

### Skip feature integration

[skip.c](../../prboom2/src/dsda/skip.c)
- `cman.h` should be included
- `dsda_HandleSkip()` function should set `demo_skiptics` variable if needed

### CLI arguments integration

[args.c](../../prboom2/src/dsda/args.c),
[args.h](../../prboom2/src/dsda/args.h)
- `arg_config` array should have `dsda_arg_cman*` entries at the very bottom
- `dsda_arg_identifier_t` enum should have `dsda_arg_cman*` entries at the very bottom (but above `dsda_arg_count` marker)

### Version message

[i_system.c](../../prboom2/src/SDL/i_system.c)
- `I_GetVersionString()` should return customized version `... (based on ...)` that references the upstream project version 

## Build system

The project is set up to build `cm-doom` executable instead of `dsda-doom` to avoid any clashes should both ports be installed.
Most of the changes listed in this section exist only to facilitate this decision. 

### CMake properties

[CMakeLists.txt](../../prboom2/CMakeLists.txt)
- `project()` should define `cm-doom` and its version
- `UPSTREAM_PROJECT_STRING` variable should be added to point to DSDA-Doom's version we are based on

[src/CMakeLists.txt](../../prboom2/src/CMakeLists.txt)
- `COMMON_SRC` should include `cman.c` and `cman.h` at the bottom
- `add_custom_command(TARGET ${TARGET} POST_BUILD ...` should reference `cm-doom` executable instead of `dsda-doom`
- `AddGameExecutable()` call at the end should have `cm-doom` instead of `dsda-doom` as its `TARGET` parameter
- Warning - `dsda-doom.exe.manifest` and `dsda-doom.wad` must be left alone!

[config.h.cin](../../prboom2/cmake/config.h.cin)
- `UPSTREAM_PROJECT_STRING` define should be added

### Visual Studio properties

[vcpkg.json](../../prboom2/vcpkg.json)
- `name` should be `cm-doom`
- `version` should match cm-doom's version
- `homepage` should link to this repo

### Packaging

[dsda-doom.desktop](../../prboom2/ICONS/dsda-doom.desktop)
- `Name` should be `cm-doom`
- `TryExec` and `Exec` should reference `cm-doom` as executable
- `StartupWMClass` should be `cm-doom`

[LinuxGenerator.cmake](../../prboom2/packaging/LinuxGenerator.cmake),
[DarwinGenerator.cmake](../../prboom2/packaging/DarwinGenerator.cmake)
- Executable names should be `cm-doom` instead of `dsda-doom`
- Warning - `dsda-doom.wad` must be left alone!

### Continuous Integration

[continuous_integration.yml](../../.github/workflows/continuous_integration.yml)
- `Lipo builds (universal)` step should reference `cm-doom` executables instead of `dsda-doom`

## Documentation

### Main project description

[README.md](../../README.md)
- Keep the cm-doom version as is
- Copy the DSDA-Doom version into [README_DSDA.md](../../README_DSDA.md) as is

### Build instructions

[prboom2/INSTALL](../../prboom2/INSTALL),
[building_on_windows.md](../guides/building_on_windows.md),
[building_on_macos.md](../guides/building_on_macos.md)
- Project should be referred to as "cm-doom" instead of "DSDA-Doom" to avoid confusion
- `git clone` instructions should reference this repo
- Executable names, clone target folder names, result package file names should all reference `cm-doom` instead of `dsda-doom`

### Additional docs

[docs/cm-doom/*](.)
- These are specific to cm-doom and should never cause merge conflicts
