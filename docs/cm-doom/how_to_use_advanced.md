# How to use cm-doom (advanced)

> [!TIP]
> It's recommended to read this document to get the most out of using cm-doom.
> Best to read the sections in order, as they build on each other. 

## CLI arguments reference

cm-doom introduces a few CLI arguments on top of DSDA-Doom ones:

| Parameter        | Description                                                                                                                              |
|------------------|------------------------------------------------------------------------------------------------------------------------------------------|
| `-cman <file>`   | Load a camera profile (`.cman` file) previously exported by the Cameraman editor. If not specified, Cameraman functionality is disabled. |
| `-cman_skip`     | Automatically skip to the frame when the camera becomes active (`delay` parameter in camera profile).                                    |
| `-cman_exit`     | Automatically exit as soon as the camera path is completed.                                                                              |
| `-cman_noflash`  | Disable gun flashes lighting up the environment, in case you find them distracting.                                                      |
| `-cman_headless` | Run in headless mode (no on-screen graphics output). Only really useful when paired with `-viddump`.                                     |

> [!TIP]
> You may already have some ideas on how to use the args above.
> If not, keep reading. You'll see the use cases for all of them below. 

## Exporting and playing camera profiles

It's **highly recommended** to go through the ["Player" chapter](https://github.com/borogk/cameraman/blob/main/docs/ch05.player.md)
from Cameraman's manual if you haven't already, learning to use cm-doom builds on that knowledge.

Try exporting a camera profile and playing it back in cm-doom:

```shell
cm-doom -cman export-0001.cman -iwad DOOM2.WAD -warp 1
```

If you read the Cameraman's Player chapter above, you would find that using `cm-doom`
is very similar to using `cm-player` module of Cameraman: 

```shell
# This plays the exported camera in ZDoom engine
cm-player load export-0001.cman -iwad DOOM2.WAD -warp 1

# This plays the exported camera in DSDA-Doom engine (notice "-cman" argument instead of "load")
cm-doom -cman export-0001.cman -iwad DOOM2.WAD -warp 1
```

## Using delayed cameras to capture demos

When capturing demos, it makes sense to split overall video into multiple clips, each responsible for a certain interval.

For example, your demo is 30 seconds long, and you decide to splice it like this:
1. 00:00 - 00:09
2. 00:10 - 00:15
3. 00:16 - 00:30

Starting from the second fragment, you should start using Cameraman's `delay` parameter that gets saved into `.cman` file. 
For example, `delay = 350` means that the camera would only activate once 350 gametics (exactly 10 seconds) have passed since the level start.

You can **skip rendering all the frames before the camera activates**, since you won't need to be capturing those.
Simply add `-cman_skip` parameter, for example:

```shell
cm-doom -cman export-0001.cman -iwad DOOM2.WAD -cl 2 -warp 1 -playdemo demo.lmp -cman_skip
```

## Auto-exit after the camera is done

Once you've captured a video clip, there is usually no point in keeping the game running.
cm-doom has a useful `-cman_exit` parameter:

```shell
cm-doom -cman export-0001.cman -iwad DOOM2.WAD -warp 1 -cman_exit
```

Running the above command would auto-quit the game once the camera is done moving.

## Viddump

Viddump allows capturing demo playback into video files without relying on realtime direct screen capture (OBS or similar). 

Capturing via viddump offers a consistent framerate even if your computer can't keep a steady FPS when playing normally.
The game simply takes as much time as it needs to render every frame.

> [!NOTE]
> Viddump is the most difficult feature to master by far. Because of that, the below section is noticeably longer.

> [!IMPORTANT]
> By default, viddump requires having [ffmpeg](https://ffmpeg.org/) installed and available in `PATH`. 

Here is an easy example to start with, it captures a camera profile into `video.mp4` once you exit the game:

```shell
cm-doom -cman export-0001.cman -iwad DOOM2.WAD -warp 1 -viddump video.mp4
```

Adding `-cman_skip` and `-cman_exit` drops the frames preceding the camera and quits once the camera stops moving.
As a result, the output video clip is frame-perfectly cut to only include the camerawork:

```shell
cm-doom -cman export-0001.cman -iwad DOOM2.WAD -warp 1 -viddump video.mp4 -cman_skip -cman_exit
```

We can also capture the camera and demo playback at the same time:

```shell
cm-doom -cman export-0001.cman -iwad DOOM2.WAD -warp 1 -viddump video.mp4 -cman_skip -cman_exit -playdemo demo.lmp
```

> [!NOTE]
> If you're a seasoned DSDA-Doom's viddump user, you know that it only works properly when playing back a demo using `-timedemo`.
> That is because `-timedemo` switches off frame pacing and processes each frame as fast as possible, which the video capturing routine requires.
> 
> For convenience, cm-doom tries to correct that behavior in the following ways:
> 1. If both `-cman` and `-viddump` arguments are present, frame pacing is switched off for proper video capture (no demo playback is necessary)
> 2. If `-cman`, `-viddump` and demo playback arguments are present, it always behaves as if the demo was loaded via `-timedemo`

If you want a clip with specific resolution, include `-width` and `-height` arguments:

```shell
cm-doom -cman export-0001.cman -iwad DOOM2.WAD -warp 1 -viddump video.mp4 -width 1280 -height 720
```

## Video encoding customization

Open `dsda-doom.cfg` and find "Video capture encoding settings" section: 

```text
# Video capture encoding settings
cap_soundcommand                "ffmpeg -f s16le -ar %s -ac 2 -i - -c:a libopus -y temp_a.nut"
cap_videocommand                "ffmpeg -f rawvideo -pix_fmt rgb24 -r %r -s %wx%h -i - -c:v libx264 -y temp_v.nut"
cap_muxcommand                  "ffmpeg -i temp_v.nut -i temp_a.nut -r %r -c copy -y %f"
```

This is how video capture works:
1. `cap_soundcommand` process is started and raw 16-bit PCM audio data is piped into its STDIN
2. `cap_videocommand` process is started and raw 24-bit RGB video data is piped into its STDIN
3. Before exiting the game, `cap_muxcommand` is executed to combine audio and video data into the file specified next to `-viddump` argument

Fully understanding how to customize this mess requires some `ffmpeg` knowledge and frankly is out of scope of this documentation.

Nonetheless, here are a few tips to get started:

1. `-c:a` parameter switches the audio codec, for example `-c:a aac` switches to AAC encoding
2. `-c:v` parameter switches the video codec, for example `-c:v libsvtav1` switches to AV1 codec
3. Run `ffmpeg -codecs` to see the list of all the audio and video codecs available to you
4. Google common codecs and which `ffmpeg` parameters are required to select them, customize their output quality, etc.

> [!NOTE]
> If you see errors in console output during video capture, see `video_stderr.txt` and `sound_stderr.txt` files for troubleshooting.
