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
// Wee. A generic #include. I feel so warm and fuzzy inside. :)

// #include "mss.h"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <aen, may 5>
// + added VERSION tag and made all text occurences of version use it
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef VERGE_INC
#define VERGE_INC

#define VERSION "2.6rev5"

#define TRUE 1
#define FALSE 0

extern void Sys_Error(const char* message, ...);

#include <string.h>
#include "SDL.h"

#include "a_memory.h"
#include "strk.h"

#include "console.h"
#include "engine.h"
#include "entity.h"
#include "font.h"
#include "image.h"
#include "keyboard.h"
#include "main.h"
#include "message.h"
#include "sound.h"

#include "render.h"
#include "timer.h"
#include "vfile.h"
#include "vc.h"
#include "graph.h"
#include "input.h"
#include "log.h"

extern GrDriver gfx;
extern Input input;
extern CFontController font;

extern bool bActive;

extern int hicolor;

extern char tolower(char c);

#ifndef WIN32
extern char* strlwr(char* str);
extern int strcasecmp(const char* s1,const char* s2);
#endif

#endif // VERGE_INC
