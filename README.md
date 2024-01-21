# dsda-doom-cameraman v0.27.5 (WIP)

This is a special fork of [dsda-doom](https://github.com/kraflab/dsda-doom), 
which enables playback of Cameraman profiles.

### What is Cameraman?

**Cameraman** is a tool for making short clips or movies in GZDoom, capturing gameplay and/or scenery.

Here is the main project page: [zdoom-cameraman](https://github.com/borogk/zdoom-cameraman).

### Why does this fork exist?

This fork allows playing back Cameraman profiles, but in dsda-doom engine instead of GZDoom.

The main Cameraman runs as a mod for GZDoom. Its main component is an editor, that allows to interactively draw
different paths for camera and to instantly try them out in-engine.

That editor also allows saving **camera profiles** as separate files, to load and re-play them later.

Playing these profiles in dsda-doom engine offers a few advantages:
- **Accurate demo playback.** 
  You can capture "cinematic" playthrough of almost any existing Doom speedrun,
  thanks to robust backwards compatibility.
- **Viddump.** 
  This feature, inherited from PrBoom+, allows capturing demo playback into video files 
  without relying on real time direct screen capture (OBS or similar). It means the output framerate will always
  be consistent, even if FPS was sluggish during the demo recording. And, in case your computer has spare rendering
  power, the output video is generated quicker than in real time.
- **More faithful visuals.**
  Both software and OpenGL rendering modes look much closer to the original Doom for MS-DOS.
  Original lighting in particular is challenging to reproduce in GZDoom, as well as some rendering artifacts.

### How to use

Run with the following fork-specific command line parameters:

`-cman <file>` loads a camera profile (.cman file), previously exported by the Cameraman Editor
([how to export](https://github.com/borogk/zdoom-cameraman/blob/main/docs/ch05.player.md#how-to-export-a-camera-profile-from-editor)).

`-cman_auto_skip` *(optional)* automatically skips to the frame when the camera becomes active 
(**'delay'** parameter in camera profile). Requires demo playback to work.

`-cman_auto_exit` *(optional)* automatically exits as soon as the camera path is completed.

`-cman_viddump <videofile>` *(optional)* shortcut to activate auto-skip, auto-exit and viddump at the same time.
Essentially, outputs a video that captures the camera path and nothing else. 
Requires demo playback by means of `-timedemo <demo>` to work property.

Examples:
```shell
# Runs the game with a camera profile
dsda-doom.exe -iwad DOOM2 -cl 2 -warp 1 -cman mycamera.cman

# Plays a demo alongside a camera profile
dsda-doom.exe -iwad DOOM2 -cl 2 -warp 1 -playdemo myrun.lmp -cman mycamera.cman

# Same as above, but skips to the camera playback
dsda-doom.exe -iwad DOOM2 -cl 2 -warp 1 -playdemo myrun.lmp -cman mycamera.cman -cman_auto_skip

# Outputs the demo+camera playback to a video clip, immediately exiting after it's done
dsda-doom.exe -iwad DOOM2 -cl 2 -warp 1 -timedemo myrun.lmp -cman mycamera.cman -cman_viddump myvideo.mkv
```

### How different is it from regular dsda-doom?

Not much. This fork strictly adds Cameraman functionality to upstream and doesn't remove or try to "fix" anything else.

For simplicity, pretty much all new code is in [cman.h](prboom2/src/cman.h) and [cman.c](prboom2/src/cman.c).
Any interactions between this module and the existing codebase are kept to a bare minimum.

Outside of extra Cameraman features, it should be safe to use this port in place of regular dsda-doom
for normal play, speedrunning etc. But in case you're extra worried, stick to the original dsda port and only
use this one for Cameraman stuff.

### Author and contributors

Originally created by **borogk** in 2024.

Based on **[dsda-doom](https://github.com/kraflab/dsda-doom)**, see its contributors on the respective repository page.

Original README is copied over to [README_DSDA.md](README_DSDA.md) out of courtesy.
