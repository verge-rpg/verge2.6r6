/*
Copyright (C) 1998 BJ Eirich (aka vecna)
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*
Standard Types Header
coded by aen

  mod log:
  21	December	1999	Created.
  30    November        2001    Added Rect struct (tSB)
*/

#ifndef TYPES_H
#define TYPES_H

#include "strk.h"

typedef unsigned char  u8;
typedef signed   char  s8;
typedef unsigned short u16;
typedef signed   short s16;
typedef unsigned long  u32;
typedef signed   long  s32;

struct SRect
{    
    int left;
    int top;
    int right;
    int bottom;

    SRect() {}
    SRect(int x1,int y1,int x2,int y2) :
        left(x1),   top(y1),
        right(x2),  bottom(y2)
        {}
};

struct Point
{
    int x,y;
};

#endif // TYPES_H
