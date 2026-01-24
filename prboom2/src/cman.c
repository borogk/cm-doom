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
#include "i_capture.h"
#include "g_game.h"
#include "dsda/args.h"
#include "dsda/skip.h"
#include "dsda/utility.h"
#include "dsda/mapinfo/doom.h"

#define BUFFER_SIZE 256

struct {
    float x;
    float y;
    float z;
    float a;
    float p;
    int enabled;
    int move_player;
    int hide_player;
    int notarget_player;
    int move_on_level_start;
    int no_flash;
    float zfix;
    int warp_episode;
    int warp_map;
    int kill;
} cman;

dboolean start_frame_next = true;
dboolean reset_interpolation_next = true;

// Converts ZDoom-style angle (between 0.0 and 1.0) to BAM.
angle_t CMAN_FromZDoomAngle(float a) {
    return (int) floorf((a - floorf(a)) * 65536) << FRACBITS;
}

// Converts BAM angle value into ZDoom-style.
float CMAN_ToZDoomAngle(angle_t a) {
    return (float) (1.0 / 65536 * (a >> FRACBITS));
}

void CMAN_Event(const char *name) {
    printf("[CMAN] [%s]\n", name);
}

void CMAN_Start() {
    start_frame_next = false;

    if (dsda_Arg(dsda_arg_viddump)->found && !capturing_video)
        I_Error("Video capture requested, but failed to start");

    CMAN_Event("START");
}

// Initializes Cameraman related stuff on level start (only called if Cameraman is active).
void CMAN_LevelStart(player_t *player) {
    // Reset interpolation and camera
    reset_interpolation_next = true;
    walkcamera.type = 0;

    if (cman.move_on_level_start) {
        cman.x = (float) player->mo->x / FRACUNIT;
        cman.y = (float) player->mo->y / FRACUNIT;
        cman.z = (float) player->viewz / FRACUNIT;
        cman.a = CMAN_ToZDoomAngle(player->mo->angle);
        cman.p = CMAN_ToZDoomAngle(player->mo->pitch);

        char buffer[BUFFER_SIZE];
        sprintf(buffer, "MOVE %f %f %f %f %f", cman.x, cman.y, cman.z, cman.a, cman.p);
        CMAN_Event(buffer);
    }

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

    // Cameraman is not enabled, quit without touching the camera or anything else
    if (!cman.enabled)
        return false;

    if (cman.warp_episode && cman.warp_map) {
        G_DeferedInitNew(gameskill, cman.warp_episode, cman.warp_map);
        cman.warp_episode = 0;
        cman.warp_map = 0;
        return false;
    }

    if (cman.kill)
        I_SafeExit(0);

    if (start_frame_next)
        CMAN_Start();

    // Detect the level start
    if (gametic == levelstarttic)
        CMAN_LevelStart(player);

    // Disable interpolation for one frame and abruptly jump to the camera starting position
    if (reset_interpolation_next) {
        reset_interpolation_next = false;
        R_ResetViewInterpolation();
    }

    // type=2 means 'freecam' mode (the kind controlled separately from the player model during demo playback)
    walkcamera.type = 2;
    walkcamera.x = dsda_FloatToFixed(cman.x);
    walkcamera.y = dsda_FloatToFixed(cman.y);
    walkcamera.z = dsda_FloatToFixed(cman.z);
    walkcamera.angle = CMAN_FromZDoomAngle(cman.a);
    walkcamera.pitch = CMAN_FromZDoomAngle(cman.p);

    int zfixed = false;
    fixed_t zfix_converted = dsda_FloatToFixed(cman.zfix);
    sector_t *camera_sector = R_PointInSector(walkcamera.x, walkcamera.y);
    if (walkcamera.z > camera_sector->ceilingheight - zfix_converted) {
        walkcamera.z = camera_sector->ceilingheight - zfix_converted;
        zfixed = true;
    } else if (walkcamera.z < camera_sector->floorheight + zfix_converted) {
        walkcamera.z = camera_sector->floorheight + zfix_converted;
        zfixed = true;
    }

    if (zfixed) {
        cman.z = (float) walkcamera.z / FRACUNIT;
        char buffer[BUFFER_SIZE];
        sprintf(buffer, "ZFIX %f", cman.z);
        CMAN_Event(buffer);
    }

    // Warp the player (not supported during demo playback)
    if (cman.move_player && !demoplayback) {
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

    if (cman.hide_player) {
        player->mo->flags2 |= MF2_DONTDRAW;
    } else {
        player->mo->flags2 &= ~MF2_DONTDRAW;
    }

    if (cman.notarget_player) {
        player->cheats |= CF_NOTARGET;
    } else {
        player->cheats &= ~CF_NOTARGET;
    }

    // Disable gun flashes
    if (cman.no_flash)
        player->extralight = 0;

    return true;
}

SDL_Thread *cmdthread;
int run_cmdthread;

static int CMAN_CmdThread(void) {
    int n;
    char buffer[BUFFER_SIZE];
    char cmd[BUFFER_SIZE];
    while (run_cmdthread) {
        if (!fgets(buffer, BUFFER_SIZE, stdin)) {
            return 0;
        }

        n = sscanf(buffer, "%s", cmd);
        if (n != 1) {
            continue;
        }

        char *params = buffer + strlen(cmd);
        if (!strcmp(cmd, "MOVE")) {
            float x, y, z, a, p;
            n = sscanf(params, "%f %f %f %f %f", &x, &y, &z, &a, &p);
            if (n == 5) {
                cman.x = x;
                cman.y = y;
                cman.z = z;
                cman.a = a;
                cman.p = p;
            }
        } else if (!strcmp(cmd, "MOVE_PLAYER")) {
            sscanf(params, "%d", &cman.move_player);
        } else if (!strcmp(cmd, "HIDE_PLAYER")) {
            sscanf(params, "%d", &cman.hide_player);
        } else if (!strcmp(cmd, "NOTARGET_PLAYER")) {
            sscanf(params, "%d", &cman.notarget_player);
        } else if (!strcmp(cmd, "MOVE_ON_LEVEL_START")) {
            sscanf(params, "%d", &cman.move_on_level_start);
        } else if (!strcmp(cmd, "NO_FLASH")) {
            sscanf(params, "%d", &cman.no_flash);
        } else if (!strcmp(cmd, "ZFIX")) {
            sscanf(params, "%f", &cman.zfix);
        } else if (!strcmp(cmd, "WARP")) {
            int arg1, arg2, episode, map;
            n = sscanf(params, "%d %d", &arg1, &arg2);
            if (n == 1) {
                episode = 1;
                map = arg1;
            } else if (n == 2) {
                episode = arg1;
                map = arg2;
            } else {
                continue;
            }

            if (dsda_ResolveCLEV(&episode, &map)) {
                cman.warp_episode = episode;
                cman.warp_map = map;
            }
        } else if (!strcmp(cmd, "KILL")) {
            cman.kill = 1;
            return 0;
        }
    }
}

void CMAN_Finish() {
    int s;
    run_cmdthread = 0;
    SDL_WaitThread(cmdthread, &s);
    CMAN_Event("FINISH");
}

// Meant to be called only once during the game startup.
void CMAN_Init() {
    // Disables Cameraman by default
    cman.enabled = 0;

    // Look for -cman command line argument
    dsda_arg_t *cman_arg = dsda_Arg(dsda_arg_cman);
    if (!cman_arg->found)
        return;

    cman.enabled = 1;
    cman.move_player = false;
    cman.hide_player = true;
    cman.notarget_player = true;
    cman.move_on_level_start = true;
    cman.no_flash = false;
    cman.zfix = 10.0;
    cman.warp_episode = 0;
    cman.warp_map = 0;

    run_cmdthread = 1;
    cmdthread = SDL_CreateThread(CMAN_CmdThread, "CMAN_CmdThread", 0);

    I_AtExit(CMAN_Finish, true, "CMAN_Finish", exit_priority_normal);
}
