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

// /---------------------------------------------------------------------\
// |                          The VERGE Engine                           |
// |              Copyright (C)1998 BJ Eirich (aka vecna)                |
// |                      Main Game Engine module                        |
// \---------------------------------------------------------------------/

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <tSB Nov 8>
// + HookKey is now handled in key_game, instead of keyboard.cpp (which no longer exists anyway)
//   As an added benefit, hooked keys no longer activate VC when other VC is already running.
// + removed keyboard.cpp and controls.cpp in favour of w_input.cpp. (class wrapper for DirectInput)
//   keyboard.h is just a list of keyboard codes now. :P
// <tSB Oct 31, 00>
// + Too much to list! @_@
// <tSB Oct 30, 00>
// + Minor tweaks here and there
// <tSB 5.19.00>
// + replaced WriteRaw24 with WriteBMP24
// + added support for version 4 VSPs (15bit uncompressed, from v2+i)
// <zero 5.7.99>
// + added ScreenShot() on F11
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


#define ENGINE_H
#include "verge.h"
#include "vtypes.h"
#include "misc.h"
#include "vfile.h"
#include "Util/RefPtr.h"

#include "image.h"
#include "vsp.h"
#include "tileanim.h"
#include "Map/MapFile.h"
#include "Map/LoadMap.h"

#include "engine.h"
#include "entity.h"
#include "player.h"
#include <stdio.h>

// ================================= Data ====================================
int xwin = 0, ywin = 0;                    // camera offset


// -- the map --

RefPtr<Map::MapFile> map = 0;

// -- vsp data --

RefPtr<VSP::VSP> vsp = 0;
TileAnim tileanim;

// entity stuffs

vector<MoveScript> movescripts;     // move scripts.  Parsed and ready to rock, baby.

Console console;

char numfollowers = 0;                // number of party followers
u8 follower[10];                    // maximum of 10 followers.
char laststeps[10]={ 0 };           // record of last movements
int lastent;

// -- stuff --

/*u8 movegranularity; // means nothing now, please remove
u8 movectr = 0;

u8 phantom = 0;                      // walk-through-walls*/
u8 speeddemon = 1;                   // doublespeed cheat

int bindarray[256];                // bind script offset

// ================================= Code ====================================

void LoadVSP(const char* fname)
{
    vsp = VSP::VSP::Load(gfx, fname);       // haheheheahehaehaehaeahe
    gfx.SetPalette((u8*)vsp->GetPal());

	tileanim.Reset(vsp);
}

void LoadMAP(const char *fname)
{
    VFILE* f = vopen(fname);
    if (!f)
        Sys_Error("LoadMap: Unable to open %s", fname);

    map = Map::LoadMap(f);

	if (!map)
		Sys_Error("Error loading %s", fname);

    ents.resize(256);   // blegh.  Hack.

    LoadMapVC(f);

    vclose(f);

    // Convert entities into the engine containers. (the map structures don't store information that's only used at runtime)
    for (uint i = 0; i<map->entities.size(); i++)
    {
        Entity& E = ents[i];
        Map::MapFile::Entity& e = map->entities[i];

        E.x = e.x;
        E.y = e.y;
        E.direction = e.direction;
        E.chrindex = e.chrindex;
        E.isobs = e.isobs;
        E.canobs = e.canobs;
        E.speed = e.speed;
        E.animscriptidx = e.animscriptidx;
        E.movescriptidx = e.movescriptidx;
        E.autoface = e.autoface;
        E.adjactivate = e.adjactivate;
        E.movecode=(Entity::MovePattern)e.movecode;
        E.movescript = e.movescript;
        E.step = e.step;
        E.delay = e.delay;
        E.stepcount = e.stepcount;
        E.delaycount = e.delaycount;
        E.visible = e.visible;
        E.wanderzone = e.wanderzone;
        E.wanderrect = e.wanderrect;
        E.actscript = e.actscript;
        E.description = e.description;
        E.SetMoveScript(map->movescripts[e.movescriptidx]);
    }

    entities = map->entities.size();

    LoadVSP(map->vspname.c_str());
    LoadCHRList(map->chrs);
    ExecuteEvent(0);
    PlayMusic(map->musname.c_str());
    timer_count = 0;
}

void ScreenShot()
{
    class Local
    {
    public:
        static const char* SSFileName()
        {
            static char fnamestr[255];

            int n = 0;
            FILE* f = 0;

            do
            {
                sprintf(fnamestr, "Screen %i.png", n);
                f = fopen(fnamestr, "r");
                if (!f)
                    return fnamestr;

                fclose(f);
                n++;
            } while(1);
        }
    };

    const char* fname = Local::SSFileName();

    if (gfx.bpp==1)
        WriteImage(gfx.screen, gfx.XRes(), gfx.YRes(), gfx.pal, fname);
    else
        WriteImage((u16*)gfx.screen, gfx.XRes(), gfx.YRes(), fname);

    Log::Write(va("Saved screenshot as %s", fname));
}
//---

// TODO: set up an interface so that the engine and console are both key listeners, or something.
// Then just send keys to the current listener.
void ProcessControls()
{
    if (console.IsOpen())
    {
        while (char c = input.GetKey())
            console.SendKey(input.Scan2ASCII(c));
    }
    else
    {
        while (char c = input.GetKey())
        {
            if (c == SCAN_GRAVE)
                console.Open();
            else
                if (bindarray[c])           // HookKey
                    HookKey(bindarray[c]);
        }

	    if (playeridx!=-1)
		    Player::HandlePlayer(ents[playeridx]);
    }
}

void GameTick()
{
    CheckMessages();
    input.Update();

    ProcessControls();
    if (speeddemon && input.key[SCAN_LCONTROL])
        ProcessControls();
    ProcessEntities();
}
