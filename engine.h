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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <zero 5.7.99>
// + added ScreenShot() headers
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef ENGINE_H
#define ENGINE_H

#include "vtypes.h"
#include "console.h"
#include "tileanim.h"
#include "Util\RefPtr.h"

namespace Map
{
    class MapFile;
};

namespace VSP
{
    class VSP;
};

extern int xwin, ywin;

extern RefPtr<Map::MapFile> map;
extern RefPtr<VSP::VSP> vsp;                       // tileset
extern TileAnim tileanim;
extern class Console console;
extern u8 movegranularity, phantom, speeddemon, movectr;

// <aen>
// This *MUST* have 256 elements, because of the new input code. I generate my own
// codes for control keys and others, such as ENTER, ESC, ALT, etc. And their codes are
// up in the 200's.
// <tSB>
// DirectInput uses the high 128 elements for extended keys and stuff, so it
// still has to be 256 elements, just for a different reason. ;)

extern int bindarray[256];

// -- prototypes --

extern void LoadVSP(const char *fname);
extern void LoadMAP(const char *fname);
extern void MAPswitch(void);
extern void MAPstats(void);
extern void ProcessControls(void);
extern void GameTick(void);

#endif // ENGINE_H
