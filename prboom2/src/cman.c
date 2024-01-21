/*
 * Copyright (C) 2024 borogk
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

// Input cameraman parameters.
struct
{
  int delay;
  int path_mode;
  int speed_mode;
  int angle_mode;
  int overshoot;
  int warp_player;
  int hide_player;
  int ga_buffer_len;
  float speed;
  float x0;
  float y0;
  float z0;
  float x1;
  float y1;
  float z1;
  float x2;
  float y2;
  float z2;
  float a0;
  float a1;
  float p0;
  float p1;
  float ra0;
  float ra1;
  float r0;
  float r1;
  float cx0;
  float cx1;
  float cy0;
  float cy1;
} cman;

// Output values for camera position.
struct
{
  float x;
  float y;
  float z;
  float a;
  float p;
} cman_out;

// Extra behavior settings
dboolean cman_auto_skip = false;
dboolean cman_auto_exit = false;

// Track active state to detect changes
dboolean cman_was_active = false;

// Angle buffer
struct
{
  float values[1024];
  int index;
  float sum;
} cman_angle_buffer;

// Previous tangent angle (relevant to Bezier)
float prev_tangent_angle;

// Converts ZDoom-style angle (between 0.0 and 1.0) to BAM.
angle_t CMAN_FromZDoomAngle(float a)
{
  return (int)floorf((a - floorf(a)) * 65536) << FRACBITS;
}

// Converts BAM angle value into ZDoom-style.
float CMAN_ToZDoomAngle(angle_t a)
{
  return (float)(1.0 / 65536 * (a >> FRACBITS));
}

// Length of <x,y> vector.
float CMAN_VectorLength(float x, float y)
{
  return (float)sqrt(x * x + y * y);
}

// Angle of <x,y> vector relative to origin (zero).
float CMAN_VectorAngle(float x, float y)
{
  angle_t a = R_PointToAngleEx2(0, 0, dsda_FloatToFixed(x), dsda_FloatToFixed(y));
  return CMAN_ToZDoomAngle(a);
}

// Corrects an angle, that crosses zero threshold (represents EAST) in relation to some previous angle value.
// The result should move two angles closer together.
// Examples:
//   given prev_angle=0.99, angle=0.01 is corrected to 1.01.
//   given prev_angle=0.01, angle=0.99 is corrected -0.01.
float CMAN_FixAngleCrossingEast(float angle, float prev_angle)
{
  if (angle - prev_angle < -0.5f)
  {
    // Crossed 1.0 moving counter-clockwise
    return angle + 1.f;
  }
  else if (angle - prev_angle > 0.5f)
  {
    // Crossed 0.0 moving clockwise
    return angle - 1.f;
  }

  return angle;
}

// Outputs next values for Linear path mode.
float CMAN_NextLinearValues(float t, dboolean overshoot)
{
  float progress = 0;
  if (cman.speed_mode == CMAN_SPEED_MODE_DISTANCE)
    progress = cman.speed * t / CMAN_VectorLength(cman.x1 - cman.x0, cman.y1 - cman.y0);
  else if (cman.speed_mode == CMAN_SPEED_MODE_TIME)
    progress = cman.speed ? t / cman.speed : 0;

  if (overshoot || progress < 1.f)
  {
    cman_out.x = cman.x0 + (cman.x1 - cman.x0) * progress;
    cman_out.y = cman.y0 + (cman.y1 - cman.y0) * progress;
    cman_out.z = cman.z0 + (cman.z1 - cman.z0) * progress;
    cman_out.a = cman.a0 + (cman.a1 - cman.a0) * progress;
    cman_out.p = cman.p0 + (cman.p1 - cman.p0) * progress;
  }
  else
  {
    cman_out.x = cman.x1;
    cman_out.y = cman.y1;
    cman_out.z = cman.z1;
    cman_out.a = cman.a1;
    cman_out.p = cman.p1;
  }

  if (cman.angle_mode == CMAN_ANGLE_MODE_RELATIVE)
    cman_out.a += CMAN_VectorAngle(cman.x1 - cman.x0, cman.y1 - cman.y0);

  return progress;
}

// Outputs next values for Radial path mode.
float CMAN_NextRadialValues(float t, dboolean overshoot)
{
  float progress = 0;
  if (cman.speed_mode == CMAN_SPEED_MODE_DISTANCE)
    progress = cman.speed * t / (float)fabs(cman.ra1 - cman.ra0);
  else if (cman.speed_mode == CMAN_SPEED_MODE_TIME)
    progress = cman.speed ? t / cman.speed : 0;

  float ra, r, cx, cy;
  if (overshoot || progress < 1.f)
  {
    ra = cman.ra0 + (cman.ra1 - cman.ra0) * progress;
    r = cman.r0 + (cman.r1 - cman.r0) * progress;
    cx = cman.cx0 + (cman.cx1 - cman.cx0) * progress;
    cy = cman.cy0 + (cman.cy1 - cman.cy0) * progress;
    cman_out.z = cman.z0 + (cman.z1 - cman.z0) * progress;
    cman_out.a = cman.a0 + (cman.a1 - cman.a0) * progress;
    cman_out.p = cman.p0 + (cman.p1 - cman.p0) * progress;
  }
  else
  {
    ra = cman.ra1;
    r = cman.r1;
    cx = cman.cx1;
    cy = cman.cy1;
    cman_out.z = cman.z1;
    cman_out.a = cman.a1;
    cman_out.p = cman.p1;
  }

  float ra_radian = (float)(ra * 2 * M_PI);
  cman_out.x = cx + (float)cos(ra_radian) * r;
  cman_out.y = cy + (float)sin(ra_radian) * r;

  if (cman.angle_mode == CMAN_ANGLE_MODE_RELATIVE)
    cman_out.a += CMAN_VectorAngle(cx - cman_out.x, cy - cman_out.y);

  return progress;
}

// Outputs next values for Bezier path mode.
float CMAN_NextBezierValues(float t, dboolean overshoot)
{
  float progress = cman.speed ? t / cman.speed : 0;

  if (overshoot || progress < 1.f)
  {
    float p = progress;
    float p2 = p * p;
    float omp = 1.f - p;
    float omp2 = omp * omp;

    cman_out.x = cman.x1 + omp2 * (cman.x0 - cman.x1) + p2 * (cman.x2 - cman.x1);
    cman_out.y = cman.y1 + omp2 * (cman.y0 - cman.y1) + p2 * (cman.y2 - cman.y1);
    cman_out.z = cman.z1 + omp2 * (cman.z0 - cman.z1) + p2 * (cman.z2 - cman.z1);
    cman_out.a = cman.a0 + (cman.a1 - cman.a0) * progress;
    cman_out.p = cman.p0 + (cman.p1 - cman.p0) * progress;
  }
  else
  {
    cman_out.x = cman.x2;
    cman_out.y = cman.y2;
    cman_out.z = cman.z2;
    cman_out.a = cman.a1;
    cman_out.p = cman.p1;
  }

  if (cman.angle_mode == CMAN_ANGLE_MODE_RELATIVE)
  {
    float p = (t - 1.f) / cman.speed;
    float p2 = p * p;
    float omp = 1.f - p;
    float omp2 = omp * omp;

    float prevx = cman.x1 + omp2 * (cman.x0 - cman.x1) + p2 * (cman.x2 - cman.x1);
    float prevy = cman.y1 + omp2 * (cman.y0 - cman.y1) + p2 * (cman.y2 - cman.y1);

    float tangent_angle = CMAN_VectorAngle(cman_out.x - prevx, cman_out.y - prevy);
    if (cman_was_active)
      tangent_angle = CMAN_FixAngleCrossingEast(tangent_angle, prev_tangent_angle);

    prev_tangent_angle = tangent_angle;
    cman_out.a += tangent_angle;
  }

  return progress;
}

// Outputs next values, depending on the path mode.
// The output is unbuffered, as in no angle filtering is applied.
float CMAN_NextValuesUnbuffered(float t, dboolean overshoot)
{
  if (cman.path_mode == CMAN_PATH_MODE_LINEAR)
    return CMAN_NextLinearValues(t, overshoot);
  else if (cman.path_mode == CMAN_PATH_MODE_RADIAL)
    return CMAN_NextRadialValues(t, overshoot);
  else if (cman.path_mode == CMAN_PATH_MODE_BEZIER)
    return CMAN_NextBezierValues(t, overshoot);

  return 1.f;
}

// Calculate next values for cman_angle_buffer.
void CMAN_NextBufferValues(float t)
{
  float next_buffer_t = t + 1.f * (cman.ga_buffer_len / 2);

  if (!cman_was_active)
  {
    // Fill the whole buffer first time around
    cman_angle_buffer.sum = 0;
    float buffer_t = next_buffer_t;
    for (int i = cman.ga_buffer_len - 1; i >= 0; i--)
    {
      CMAN_NextValuesUnbuffered(buffer_t, true);
      cman_angle_buffer.values[i] = cman_out.a;
      cman_angle_buffer.sum += cman_out.a;
      buffer_t -= 1.f;
    }
  }
  else
  {
    // Update only the buffer's difference
    CMAN_NextValuesUnbuffered(next_buffer_t, true);
    cman_angle_buffer.sum -= cman_angle_buffer.values[cman_angle_buffer.index];
    cman_angle_buffer.sum += cman_out.a;
    cman_angle_buffer.values[cman_angle_buffer.index] = cman_out.a;
  }

  // Advance the cyclic buffer index
  cman_angle_buffer.index++;
  if (cman_angle_buffer.index == cman.ga_buffer_len)
    cman_angle_buffer.index = 0;
}

// Outputs next values, depending on the path mode.
// Applies angle filtering if needed.
float CMAN_NextValues(float t)
{
  dboolean angle_buffering_needed =
    cman.ga_buffer_len > 1 &&
    cman.path_mode == CMAN_PATH_MODE_BEZIER &&
    cman.angle_mode == CMAN_ANGLE_MODE_RELATIVE;

  // Calculate angle buffer data first to not mess with the output below
  if (angle_buffering_needed)
    CMAN_NextBufferValues(t);

  // Calculate next positional and angle values, this should output to cman_out
  float progress = CMAN_NextValuesUnbuffered(t, cman.overshoot);

  // Adjust the angle using the buffer if needed
  if (angle_buffering_needed)
    cman_out.a = cman_angle_buffer.sum / cman.ga_buffer_len;

  return progress;
}

// Meant to be called every gametic from P_WalkTicker.
// Returns true when Cameraman is engaged, this should tell P_WalkTicker back the camera control is overridden.
int CMAN_Ticker()
{
  // Cameraman is not loaded at all, quit without touching the camera or anything else
  if (cman.delay < 0)
    return false;

  // Reset the camera at every level start
  if (gametic == levelstarttic)
  {
    walkcamera.type = 0;
    cman_was_active = false;
  }

  // Cameraman time must be exactly 0 after the current level has started and 'delay' tics have passed
  // Don't start earlier than that
  int cman_time = leveltime - cman.delay - 1;
  if (cman_time < 0)
    return false;

  // Calculate next camera values
  float t = (float)cman_time;
  float progress = CMAN_NextValues(t);

  // Update the camera values as long as the camera path is not completed
  if (progress < 1.f)
  {
    // Disable interpolation for one frame and abruptly jump to the camera starting position
    if (!cman_was_active)
      R_ResetViewInterpolation();

    // type=2 means 'freecam' mode (the kind controlled separately from the player model during demo playback)
    walkcamera.type = 2;
    walkcamera.x = dsda_FloatToFixed(cman_out.x);
    walkcamera.y = dsda_FloatToFixed(cman_out.y);
    walkcamera.z = dsda_FloatToFixed(cman_out.z);
    walkcamera.angle = CMAN_FromZDoomAngle(cman_out.a);
    walkcamera.pitch = CMAN_FromZDoomAngle(cman_out.p);

    // Player mobj to manipulate if needed
    mobj_t* player = players[displayplayer].mo;

    // Warp the player (not supported during demo playback)
    if (cman.warp_player && !demoplayback)
    {
      P_MapStart();

      if (P_TeleportMove(player, walkcamera.x, walkcamera.y, false))
      {
        player->z = walkcamera.z;
        player->angle = walkcamera.angle;
        player->pitch = walkcamera.pitch;
        player->momx = 0;
        player->momy = 0;
        player->momz = 0;
      }

      P_MapEnd();
    }

    // Hide the player
    if (cman.hide_player)
      player->flags2 |= MF2_DONTDRAW;
  }
  else
  {
    // Auto-exit after the camera is done, but not while skipping frames
    // The skip mode check prevents premature exits, e.g. when skipping a level in multi-level demos
    if (cman_auto_exit && !dsda_SkipMode())
      I_SafeExit(0);
  }

  cman_was_active = true;
  return true;
}

// Sets notable default input parameters.
void CMAN_InitDefaults()
{
  cman.delay = 0;
  cman.path_mode = CMAN_PATH_MODE_LINEAR;
  cman.speed_mode = CMAN_SPEED_MODE_DISTANCE;
  cman.angle_mode = CMAN_ANGLE_MODE_RELATIVE;
  cman.overshoot = false;
  cman.warp_player = false;
  cman.hide_player = false;
  cman.speed = 1.f;
}

// Meant to be called only once during the game startup.
void CMAN_Init()
{
  // Disables Cameraman by default
  cman.delay = -1;

  // Look for -cman command line argument
  dsda_arg_t *cman_arg = dsda_Arg(dsda_arg_cman);
  if (!cman_arg->found)
    return;

  // Look for -cman_auto_skip command line argument
  if (dsda_Flag(dsda_arg_cman_auto_skip))
    cman_auto_skip = true;

  // Look for -cman_auto_exit command line argument
  if (dsda_Flag(dsda_arg_cman_auto_exit))
    cman_auto_exit = true;

  // Look for -cman_viddump command line argument
  dsda_arg_t *cman_viddump_arg = dsda_Arg(dsda_arg_cman_viddump);
  if (cman_viddump_arg->found)
  {
    cman_auto_skip = true;
    cman_auto_exit = true;
    dsda_UpdateStringArg(dsda_arg_viddump, cman_viddump_arg->value.v_string);
  }

  CMAN_InitDefaults();

  char* cman_file;
  char* line = Z_Malloc(CMAN_CONFIG_BUFFER_SIZE);
  char param_name[64];
  char param_separator;
  float param_value;

  cman_file = Z_Strdup(cman_arg->value.v_string);
  cman_file = I_RequireFile(cman_file, ".cman");
  lprintf(LO_INFO, "Loading Cameraman profile: %s\n", cman_file);

  // Parse .cman file line-by-line
  // Each line is expected to be '<param> = <value>'
  // Unrecognized lines and param names are ignored
  FILE* f = M_OpenFile(cman_file, "r");
  Z_Free(cman_file);

  if (f)
  {
    while (!feof(f))
    {
      line = fgets(line, CMAN_CONFIG_BUFFER_SIZE, f);

      if (!line)
        break;

      if (sscanf(line, "%63s %c %f\n", param_name, &param_separator, &param_value) != 3 || param_separator != '=')
        continue;

      lprintf(LO_DEBUG, " Cameraman param: %s = %f\n", param_name, param_value);

      if (!strcmp(param_name, "path_mode"))
        cman.path_mode = (int)param_value;
      else if (!strcmp(param_name, "speed_mode"))
        cman.speed_mode = (int)param_value;
      else if (!strcmp(param_name, "angle_mode"))
        cman.angle_mode = (int)param_value;
      else if (!strcmp(param_name, "delay"))
        cman.delay = (int)param_value;
      else if (!strcmp(param_name, "overshoot"))
        cman.overshoot = (int)param_value;
      else if (!strcmp(param_name, "warp_player"))
        cman.warp_player = (int)param_value;
      else if (!strcmp(param_name, "hide_player"))
        cman.hide_player = (int)param_value;
      else if (!strcmp(param_name, "ga_buffer_len"))
        cman.ga_buffer_len = (int)param_value;
      else if (!strcmp(param_name, "speed"))
        cman.speed = param_value;
      else if (!strcmp(param_name, "x0"))
        cman.x0 = param_value;
      else if (!strcmp(param_name, "y0"))
        cman.y0 = param_value;
      else if (!strcmp(param_name, "z0"))
        cman.z0 = param_value;
      else if (!strcmp(param_name, "x1"))
        cman.x1 = param_value;
      else if (!strcmp(param_name, "y1"))
        cman.y1 = param_value;
      else if (!strcmp(param_name, "z1"))
        cman.z1 = param_value;
      else if (!strcmp(param_name, "x2"))
        cman.x2 = param_value;
      else if (!strcmp(param_name, "y2"))
        cman.y2 = param_value;
      else if (!strcmp(param_name, "z2"))
        cman.z2 = param_value;
      else if (!strcmp(param_name, "a0"))
        cman.a0 = param_value;
      else if (!strcmp(param_name, "a1"))
        cman.a1 = param_value;
      else if (!strcmp(param_name, "p0"))
        cman.p0 = param_value;
      else if (!strcmp(param_name, "p1"))
        cman.p1 = param_value;
      else if (!strcmp(param_name, "ra0"))
        cman.ra0 = param_value;
      else if (!strcmp(param_name, "ra1"))
        cman.ra1 = param_value;
      else if (!strcmp(param_name, "r0"))
        cman.r0 = param_value;
      else if (!strcmp(param_name, "r1"))
        cman.r1 = param_value;
      else if (!strcmp(param_name, "cx0"))
        cman.cx0 = param_value;
      else if (!strcmp(param_name, "cy0"))
        cman.cy0 = param_value;
      else if (!strcmp(param_name, "cx1"))
        cman.cx1 = param_value;
      else if (!strcmp(param_name, "cy1"))
        cman.cy1 = param_value;
    }

    fclose(f);
  }

  Z_Free(line);
}

// Meant to be called when setting up skiptics. Returns amount of tics to skip or -1 if no skip is needed.
int CMAN_SkipTics()
{
  return cman_auto_skip ? cman.delay : -1;
}
