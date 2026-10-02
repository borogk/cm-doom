# cm-doom

This is a special fork of [DSDA-Doom](https://github.com/kraflab/dsda-doom) source port that enables playback of [Cameraman](https://github.com/borogk/cameraman) profiles.

**Cameraman** allows interactively drawing camera paths and instantly trying them out in-engine.
It also allows saving **camera profiles** as separate files. These camera profiles can then be loaded into **cm-doom** for playback in the DSDA-Doom engine.

## Downloads

| File name                                                                                                                                    | Platform |
|----------------------------------------------------------------------------------------------------------------------------------------------|----------|
| [cm-doom-1.1.0-win-x64.zip](https://github.com/borogk/cm-doom/releases/download/v1.1.0-alpha1/cm-doom-1.1.0-win-x64.zip)                     | Windows  |
| [cm-doom-1.1.0-linux-x86_64.appimage](https://github.com/borogk/cm-doom/releases/download/v1.1.0-alpha1/cm-doom-1.1.0-linux-x86_64.appimage) | Linux    |
| [cm-doom-1.1.0-mac-uni.zip](https://github.com/borogk/cm-doom/releases/download/v1.1.0-alpha1/cm-doom-1.1.0-mac-uni.zip)                     | macOS    |

## How to use

> [!IMPORTANT]
> **This source port requires knowledge of how to use Cameraman!**
> 
> You are meant to use **cm-doom** in tandem with **Cameraman**, therefore: 
> 
> 1. Visit [Cameraman's page on GitHub](https://github.com/borogk/cameraman) for installation and usage instructions
> 2. Knowing how to draw and play camera paths isn't enough, make sure you understand [how to export camera profiles](https://github.com/borogk/zdoom-cameraman/blob/main/docs/ch05.player.md#how-to-export-a-camera-profile-from-editor)
> 3. Basic terminal usage skills are required, enough to be comfortable using parameters like `-iwad`, `-file`, `-playdemo` etc.

Quick start example:

```shell
# Runs the game with a camera profile
cm-doom -iwad DOOM2.WAD -cl 2 -warp 1 -cman export-0001.cman
```

More information can be found on the [How to use cm-doom (advanced)](docs/cm-doom/how_to_use_advanced.md) page. 

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

The exhaustive list of changes can be found on [Changes compared to upstream](docs/cm-doom/changes_compared_to_upstream.md) page.

## Future support strategy

The current plan is to focus on **only developing things related to Cameraman.**
All other functionality will be regularly pulled from upstream DSDA-Doom releases as is.

## Author and contributors

Originally created by **borogk** in 2024.

Based on [DSDA-Doom](https://github.com/kraflab/dsda-doom), see its contributors on the respective repository page.

Original README is copied over to [README_DSDA.md](README_DSDA.md) out of courtesy.

Big thanks to [Vytaan](https://www.youtube.com/@Vytaan) for testing.
