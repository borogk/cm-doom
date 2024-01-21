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
#include "r_main.h"
#include "lprintf.h"
#include "dsda/args.h"
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

// Outputs next values for Linear path mode.
void CMAN_NextLinearValues(float t)
{
  float progress;
  if (cman.speed_mode == CMAN_SPEED_MODE_DISTANCE)
  {
    progress = cman.speed * t / CMAN_VectorLength(cman.x1 - cman.x0, cman.y1 - cman.y0);
  }
  else if (cman.speed_mode == CMAN_SPEED_MODE_TIME)
  {
    progress = t / cman.speed;
  }

  if (cman.overshoot || progress < 1.f)
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
  {
    cman_out.a += CMAN_VectorAngle(cman.x1 - cman.x0, cman.y1 - cman.y0);
  }
}

// Outputs next values for Radial path mode.
void CMAN_NextRadialValues(float t)
{
  float progress;
  if (cman.speed_mode == CMAN_SPEED_MODE_DISTANCE)
  {
    progress = cman.speed * t / (float)fabs(cman.ra1 - cman.ra0);
  }
  else if (cman.speed_mode == CMAN_SPEED_MODE_TIME)
  {
    progress = t / cman.speed;
  }

  float ra, r, cx, cy;
  if (cman.overshoot || progress < 1.f)
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
  {
    cman_out.a += CMAN_VectorAngle(cx - cman_out.x, cy - cman_out.y);
  }
}

// Outputs next values for Bezier path mode.
void CMAN_NextBezierValues(float t)
{
  float progress = t / cman.speed;

  if (cman.overshoot || progress < 1.f)
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

    float tangentAngle = CMAN_VectorAngle(cman_out.x - prevx, cman_out.y - prevy);
    cman_out.a += tangentAngle;
  }
}

// Meant to be called every gametic from P_WalkTicker.
// Returns true when Cameraman is engaged, this should tell P_WalkTicker back the camera control is overridden.
int CMAN_Ticker()
{
  // Cameraman is not loaded at all
  if (cman.delay < 0)
    return false;

  // Cameraman is loaded, but the starting tic hasn't come yet
  if (leveltime < cman.delay)
    return false;

  // Calculate next camera values depending on the path mode
  float t = (float)(leveltime - cman.delay);
  if (cman.path_mode == CMAN_PATH_MODE_LINEAR)
    CMAN_NextLinearValues(t);
  else if (cman.path_mode == CMAN_PATH_MODE_RADIAL)
    CMAN_NextRadialValues(t);
  else if (cman.path_mode == CMAN_PATH_MODE_BEZIER)
    CMAN_NextBezierValues(t);

  // Set the camera values
  // type=2 means 'freecam' mode (the kind controlled separately from the player model during demo playback)
  walkcamera.type = 2;
  walkcamera.x = dsda_FloatToFixed(cman_out.x);
  walkcamera.y = dsda_FloatToFixed(cman_out.y);
  walkcamera.z = dsda_FloatToFixed(cman_out.z);
  walkcamera.angle = CMAN_FromZDoomAngle(cman_out.a);
  walkcamera.pitch = CMAN_FromZDoomAngle(cman_out.p);

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
  dsda_arg_t *arg = dsda_Arg(dsda_arg_cman);
  if (!arg->found)
    return;

  CMAN_InitDefaults();

  char* cman_file;
  char* line = Z_Malloc(CMAN_CONFIG_BUFFER_SIZE);
  char param_name[64];
  char param_separator;
  float param_value;

  cman_file = Z_Strdup(arg->value.v_string);
  cman_file = I_RequireFile(cman_file, ".cman");
  lprintf(LO_INFO, "Loading Cameraman profile: %s\n", cman_file);

  // Parse .cman file line-by-line
  // Each line is expected to be '<param> = <value>'
  // Unrecognized lines and param names are ignored
  FILE* f = M_OpenFile(cman_file, "r");
  if (f)
  {
    while (!feof(f))
    {
      line = fgets(line, CMAN_CONFIG_BUFFER_SIZE, f);

      if (!line)
        break;

      if (sscanf(line, "%s %c %f\n", param_name, &param_separator, &param_value) != 3 || param_separator != '=')
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
