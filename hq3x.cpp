//hq3x C++ end
//----------------------------------------------------------
//Copyright (C) 2003 MaxSt ( maxst@hiend3d.com )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later
//version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "hq3x.h"

unsigned int   LUT16to32[65536];
unsigned int   RGBtoYUV[65536];

void InitLUTs()
{
    for (int i = 0; i < 65536; i++)
        LUT16to32[i] = ((i & 0xF800) << 8) + ((i & 0x07E0) << 5) + ((i & 0x001F) << 3);

    for (int i = 0; i < 32; i++)
        for (int j = 0; j < 64; j++)
            for (int k = 0; k < 32; k++)
            {
                int r = i << 3;
                int g = j << 2;
                int b = k << 3;
                int Y = (r + g + b) >> 2;
                int u = 128 + ((r - b) >> 2);
                int v = 128 + ((-r + 2*g -b)>>3);
                RGBtoYUV[ (i << 11) + (j << 5) + k ] = (Y<<16) + (u<<8) + v;
            }
}
