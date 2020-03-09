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

#ifndef IMAGE_INC
#define IMAGE_INC

#include "vtypes.h"

extern int		Image_Width();
extern int		Image_Length();

u8*	Image_LoadBuf(const char* filename);
void WriteImage(u8* data, int width, int height, u8* palette, const char* filename);
void WriteImage(u16* data, int width, int height, const char* filename);

#endif // IMAGE_INC
