/*
 * Copyright (C) 2026 borogk
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *  Cameraman module.
 */

#include <math.h>

#include "SDL.h"
#include "SDL_thread.h"

#include "cman.h"
#include "e6y.h"
#include "doomstat.h"
#include "i_system.h"
#include "m_file.h"
#include "i_main.h"
#include "r_main.h"
#include "r_fps.h"
#include "p_map.h"
#include "lprintf.h"
#include "dsda/args.h"
#include "dsda/skip.h"
#include "dsda/utility.h"

#define CMAN_PATH_MODE_LINEAR       0
#define CMAN_PATH_MODE_RADIAL       1
#define CMAN_PATH_MODE_BEZIER       2
#define CMAN_SPEED_MODE_DISTANCE    0
#define CMAN_SPEED_MODE_TIME        1
#define CMAN_ANGLE_MODE_RELATIVE    0
#define CMAN_ANGLE_MODE_ABSOLUTE    1

#define CMAN_CONFIG_BUFFER_SIZE     1024

struct {
    float x;
    float y;
    float z;
    float a;
    float p;
    int delay;
    int warp_player;
    int hide_player;
    int no_flash;
} cman;

// Track active state to detect changes
dboolean cman_was_active = false;

// Converts ZDoom-style angle (between 0.0 and 1.0) to BAM.
angle_t CMAN_FromZDoomAngle(float a) {
    return (int) floorf((a - floorf(a)) * 65536) << FRACBITS;
}

// Initializes Cameraman related stuff on level start (only called if Cameraman is active).
void CMAN_LevelStart() {
    // Reset active flag
    cman_was_active = false;

    // Reset the camera
    walkcamera.type = 0;

    // Implementation of auto-skip for when a demo is not playing
    //  int cman_skiptics = CMAN_SkipTics();
    //  if (cman_skiptics > 0 && !demoplayback)
    //    dsda_SkipToLogicTic(true_logictic + cman_skiptics);
}

// Meant to be called every gametic from P_WalkTicker.
// Returns true when Cameraman is engaged, this should tell P_WalkTicker back the camera control is overridden.
int CMAN_Ticker() {
    // Player mobj to manipulate if needed
    player_t *player = &players[displayplayer];

    // Cameraman is not loaded at all, quit without touching the camera or anything else
    if (cman.delay < 0)
        return false;

    // Detect the level start
    if (gametic == levelstarttic)
        CMAN_LevelStart();

    // Cameraman time must be exactly 0 after the current level has started and 'delay' tics have passed
    // Don't start earlier than that
    int cman_time = leveltime - cman.delay - 1;
    if (cman_time < 0)
        return false;

    // Disable interpolation for one frame and abruptly jump to the camera starting position
    if (!cman_was_active)
        R_ResetViewInterpolation();

    // type=2 means 'freecam' mode (the kind controlled separately from the player model during demo playback)
    walkcamera.type = 2;
    walkcamera.x = dsda_FloatToFixed(cman.x);
    walkcamera.y = dsda_FloatToFixed(cman.y);
    walkcamera.z = dsda_FloatToFixed(cman.z);
    walkcamera.angle = CMAN_FromZDoomAngle(cman.a);
    walkcamera.pitch = CMAN_FromZDoomAngle(cman.p);

    // Warp the player (not supported during demo playback)
    if (cman.warp_player && !demoplayback) {
        P_MapStart();

        if (P_TeleportMove(player->mo, walkcamera.x, walkcamera.y, false)) {
            player->mo->z = walkcamera.z;
            player->mo->angle = walkcamera.angle;
            player->mo->pitch = walkcamera.pitch;
            player->mo->momx = 0;
            player->mo->momy = 0;
            player->mo->momz = 0;
        }

        P_MapEnd();
    }

    // Hide the player
    if (cman.hide_player)
        player->mo->flags2 |= MF2_DONTDRAW;

    // Disable gun flashes
    if (cman.no_flash)
        player->extralight = 0;

    cman_was_active = true;
    return true;
}

SDL_Thread *cmdthread;

static int CMAN_CmdThread(void *data) {
    int n;
    char command[128];
    while (true) {
        n = scanf("%s", command);
        if (n == 1) {
            if (!strcmp(command, "MOVE")) {
                float x, y, z, a, p;
                n = scanf("%f %f %f %f %f", &x, &y, &z, &a, &p);
                if (n == 5) {
                    cman.x = x;
                    cman.y = y;
                    cman.z = z;
                    cman.a = a;
                    cman.p = p;
                }
            }
        }
    }
}

// Meant to be called only once during the game startup.
void CMAN_Init() {
    // Disables Cameraman by default
    cman.delay = -1;

    // Look for -cman command line argument
    dsda_arg_t *cman_arg = dsda_Arg(dsda_arg_cman);
    if (!cman_arg->found)
        return;

    cman.delay = 0;
    cman.warp_player = false;
    cman.hide_player = false;
    cman.no_flash = false;

    int data;
    cmdthread = SDL_CreateThread(CMAN_CmdThread, "CMAN_CmdThread", &data);
}
