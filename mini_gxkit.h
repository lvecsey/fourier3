/*
  Graphics Kit library
  Copyright (C) 2015  Lester Vecsey

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef MINI_GXKIT_H
#define MINI_GXKIT_H

typedef struct {
  double x,y;
} point_t;

typedef struct {
  double x,y,z;
} point3d_t;

#include <sys/types.h>
#include <stdint.h>

typedef struct {
  uint16_t r,g,b;
} pixel_t;

typedef struct {
  uint32_t r,g,b;
} pixel32_t;

typedef struct {
  uint64_t r,g,b;
} pixel64_t;

typedef struct {
  pixel_t *rgb;
  long int xres, yres;
} image_t;

#endif
