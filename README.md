# cm-doom

This is a special fork of [DSDA-Doom](https://github.com/kraflab/dsda-doom) source port that enables playback of [Cameraman](https://github.com/borogk/cameraman) profiles.

**Cameraman** allows interactively drawing camera paths and instantly trying them out in-engine.
It also allows saving **camera profiles** as separate files. These camera profiles can then be loaded into **cm-doom** for playback in the DSDA-Doom engine.

## Installation

| Platform | Instructions                                                                                                                                                                                                                                                             |
|----------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Windows  | Download pre-built binaries [windows-cm-doom-1.0.0.zip](https://github.com/borogk/cm-doom/releases/download/v1.0.0/windows-cm-doom-1.0.0.zip). If you want to compile from sources instead, follow [build instructions for Windows](docs/guides/building_on_windows.md). |
| Linux    | Follow [build instructions for Linux](prboom2/INSTALL).                                                                                                                                                                                                                  |
| macOS    | Follow [build instructions for macOS](docs/guides/building_on_macos.md).                                                                                                                                                                                                 |

## How to use

> [!IMPORTANT]
> **This source port requires knowledge of how to use Cameraman!**
> 
> You are meant to use **cm-doom** in tandem with **Cameraman**, therefore: 
> 
> 1. Visit [Cameraman's page on GitHub](https://github.com/borogk/cameraman) for installation and usage instructions
> 2. Knowing how to draw and play camera paths isn't enough, make sure you understand [how to export camera profiles](https://github.com/borogk/zdoom-cameraman/blob/main/docs/ch05.player.md#how-to-export-a-camera-profile-from-editor)
> 3. Basic terminal usage skills are required, enough to be comfortable using parameters like `-iwad`, `-file`, `-playdemo` etc.

Run cm-doom with the following fork-specific command line parameters (all are optional):

| Parameter        | Description                                                                                                                                                                             |
|------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `-cman <file>`   | Load a camera profile (`.cman` file) previously exported by the Cameraman editor. If not specified, Cameraman functionality is disabled and all parameters described below are ignored. |
| `-cman_skip`     | Automatically skip to the frame when the camera becomes active (`delay` parameter in camera profile).                                                                                   |
| `-cman_exit`     | Automatically exit as soon as the camera path is completed.                                                                                                                             |
| `-cman_noflash`  | Disable gun flashes lighting up the environment, in case you find them distracting.                                                                                                     |
| `-cman_headless` | Run in headless mode (no on-screen graphics output). Only really useful when paired with `-viddump`.                                                                                    |

Examples:

```shell
# Runs the game with a camera profile
cm-doom -iwad DOOM2 -cl 2 -warp 1 -cman export-0001.cman

# Plays a demo alongside a camera profile
cm-doom -iwad DOOM2 -cl 2 -warp 1 -playdemo demo.lmp -cman export-0001.cman

# Same as above, but skips to the camera playback
cm-doom -iwad DOOM2 -cl 2 -warp 1 -playdemo demo.lmp -cman export-0001.cman -cman_skip

# Outputs the demo+camera playback to a video clip, immediately exiting after it's done
cm-doom -iwad DOOM2 -cl 2 -warp 1 -timedemo demo.lmp -cman export-0001.cman -cman_skip -cman_exit -viddump vid.mkv
```

## Advantages

Playing Cameraman profiles in DSDA-Doom engine offers a few advantages over ZDoom:

1. **Accurate demo playback.**
   You can capture "cinematic" playthrough of almost any existing Doom speedrun,
   thanks to robust backwards compatibility.
2. **Viddump.**
   This feature, inherited from PrBoom+, allows capturing demo playback into video files without relying on
   realtime direct screen capture (OBS or similar). It means the output framerate will always be consistent,
   even if FPS was sluggish during the demo recording.
3. **More faithful visuals.**
   Both software and OpenGL rendering modes look much closer to the original Doom for MS-DOS.
   Original lighting in particular is challenging to reproduce in ZDoom variants, as well as some rendering artifacts.

## How different is cm-doom from DSDA-Doom?

This fork strictly adds a bit of functionality to upstream without removing or "fixing" anything.

Outside of extra Cameraman features, it should be safe to use this port in place of regular DSDA-Doom
for normal play, speedrunning, etc. But in case you're extra worried, stick to the original DSDA port
and only use cm-doom for Cameraman stuff.

The exhaustive list of changes can be found on [Changes compared to upstream](docs/cm-doom/changes-compared-to-upstream.md) page.

## Future support strategy

The current plan is to focus on **only developing things related to Cameraman.**
All other functionality will be regularly pulled from upstream DSDA-Doom releases as is.

## Author and contributors

Originally created by **borogk** in 2024.

Based on [DSDA-Doom](https://github.com/kraflab/dsda-doom), see its contributors on the respective repository page.

Original README is copied over to [README_DSDA.md](README_DSDA.md) out of courtesy.

Big thanks to [Vytaan](https://www.youtube.com/@Vytaan) for testing.
