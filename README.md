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

TBD

### Author and contributors

Originally created by **borogk** in 2024.

Based on **[dsda-doom](https://github.com/kraflab/dsda-doom)**, see its contributors on the respective repository page.

Original README is copied over to [README_DSDA.md](README_DSDA.md) out of courtesy.
