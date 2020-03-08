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

// ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
// ³                          The VERGE Engine                           ³
// ³              Copyright (C)1998 BJ Eirich (aka vecna)                ³
// ³                      Main Game Engine module                        ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <tSB Nov 8>
// + HookKey is now handled in key_game, instead of keyboard.cpp (which no longer exists anyway)
//   As an added benefit, hooked keys no longer activate VC when other VC is already running.
// + removed keyboard.cpp and controls.cpp in favour of w_input.cpp. (class wrapper for DirectInput)
//   keyboard.h is just a list of keyboard codes now. :P
// <tSB Oct 31,00>
// + Too much to list! @_@
// <tSB Oct 30,00>
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
#include "vfile.h"
#include "vsp.h"
#include "engine.h"
#include "misc.h"
#include "player.h"
#include "image.h"
#include <stdio.h>

// ================================= Data ====================================

typedef struct
{
    char pmultx,pdivx;                 // parallax multiplier/divisor for X
    char pmulty,pdivy;                 // parallax multiplier/divisor for Y
    unsigned short sizex, sizey;       // layer dimensions.
    unsigned char trans, hline;        // transparency flag | hline (raster fx)
} layer_r;

typedef struct
{
    char name[40];                     // zone name/desc
    unsigned short script;             // script to call thingy
    unsigned short percent;            // chance of executing
    unsigned short delay;              // step-delay
    unsigned short aaa;                // Accept Adjacent Activation
    unsigned short entityscript;       // script to call for entities
} zoneinfo;

zoneinfo zones[256];                   // zone data records
layer_r layer[6];                      // Array of layer data
//vspanim_r vspanim[100];                // tile animation data
unsigned short vadelay[100];           // Tile animation delay ctr

char mapname[60+1];                    // MAP filename
char vspname[60+1];                    // VSP filemap
char musname[60+1];                    // MAP bkgrd music default filename
string_k rstring;                      // render order
char numlayers;                        // number of layers in map
u8 *obstruct=0;
u8 *zone=0;                          // obstruction and zone buffers
char layertoggle[8];                   // layer visible toggles
u16 xstart, ystart;                   // MAP start x/y location
int bufsize;                           // how many bytes need to be written
int numzones;                          // number of active zones

u16 *layers[6];                       // Raw layer data
int xwin=0, ywin=0;                    // camera offset

// -- vsp data --

VSP::VSP* vsp=0;
Console console;

// entity stuffs

vector<MoveScript> movescripts;     // move scripts.  Parsed and ready to rock, baby.

char numfollowers=0;                // number of party followers
u8 follower[10];                    // maximum of 10 followers.
char laststeps[10]={ 0 };           // record of last movements
int lastent;

// -- stuff --

u8 movegranularity; // means nothing now, please remove
u8 movectr=0;

u8 phantom=0;                      // walk-through-walls
u8 speeddemon=1;                   // doublespeed cheat

// <aen>
// This *MUST* have 256 elements, because of the new input code. I generate my own
// codes for control keys and others, such as ENTER, ESC, ALT, etc. And their codes are
// up in the 200's.
// <tSB>
// Still needs 256 elements, but because of DirectInput.

int bindarray[256];                  // bind script offset

// ================================= Code ====================================

int ReadCompressedLayer1(u8* dest, int len, char *buf)
{
    u8 run, w;
    
    do
    {
        run = 1;
        w = *buf++;
        if (0xFF == w)
        {
            run = *buf++;
            w = *buf++;
        }
        len -= run;
        // totally bogus. shaa.
        if (len < 0)
            return 1;
        while (run--)
            *dest++ = w;
    }
    while (len);
    
    // good
    return 0;
}

int ReadCompressedLayer2(u16* dest, int len, u16 *buf)
{
    u16 run, w;
    
    do
    {
        run = 1;
        w = *buf++;
        if ((w & 0xFF00) == 0xFF00)
        {
            run = (u16)(w & 0x00FF);
            w = *buf++;
        }
        len -= run;
        // totally bogus. shaa.
        if (len < 0)
            return 1;
        while (run--)
            *dest++ = w;
    }
    while (len);
    
    // good
    return 0;
}

void LoadVSP(const char* fname)
{
    vsp=VSP::VSP::LoadVSP(gfx,fname);       // haheheheahehaehaehaeahe
    gfx.SetPalette((u8*)vsp->GetPal());
}

void FreeVSP()
{
    delete vsp;
    vsp=0;

    if (mapvc!=NULL)
    {
        vfree(mapvc);     mapvc=NULL;
    }
}

void LoadMAP(const char *fname)
{
    VFILE*    f;
    char*    cb;
    int        i;
    int        bogus; // for checking ReadCompressedLayer*
    char    sig[6+1];
    char    rstring_temp[20+1];
    
    // No matter where you go, you're there.
    
    Log::Write(va("Loading MAP %s.", fname));
    strncpy(mapname, fname, 60);
    mapname[60]='\0';
    
    f    =vopen(fname);
    if (!f)
        Sys_Error("Could not find %s.", fname);
    
    vread(sig, 6, f);
    sig[6]='\0';
    if (strcmp(sig, "MAPù5"))
        Sys_Error("%s is not a recognized MAP file.", fname);
    
    // Lalala! Can you find Waldo hiding in the code? Here's a hint, he likes to dress like a candy-cane.
    
    vread(&i, 4, f);
    vread(vspname, 60, f);
    vread(musname, 60, f);
    
    vread(rstring_temp, 20, f);
    rstring_temp[20]='\0';
    rstring=rstring_temp;
    
    vread(&xstart, 2, f);
    vread(&ystart, 2, f);
    vseek(f, 51, SEEK_CUR);
    //vread(strbuf, 51, f);
    
    vread(&numlayers, 1, f);
    for (i=0; i<numlayers; i++)
        vread(&layer[i], 12, f);
    
    memset(layertoggle, 0, 8);
    // read actual layer data.
    for (i=0; i<numlayers; i++)
    {
        vread(&bufsize, 4, f);
        layers[i]=(unsigned short *) valloc(layer[i].sizex*(layer[i].sizey+2)*2, "LoadMAP:layers[i]", OID_MAP);
        cb=(char *) valloc(bufsize, "LoadMAP:cb", OID_TEMP);
        vread(cb, bufsize, f);
        bogus=ReadCompressedLayer2(layers[i],(layer[i].sizex * layer[i].sizey), (u16 *) cb);
        if (bogus)
        {
            Sys_Error("LoadMAP: %s: bogus compressed layer data (layer %d)", fname, i);
        }
        vfree(cb);
        layertoggle[i]=1;
    }
    
    obstruct=(u8 *) valloc(layer[0].sizex*(layer[0].sizey+2), "obstruct", OID_MAP);
    zone=(u8 *) valloc(layer[0].sizex*(layer[0].sizey+2), "zone", OID_MAP);
    
    // read obstruction grid
    vread(&bufsize, 4, f);
    cb=(char *) valloc(bufsize, "LoadMAP:cb (2)", OID_TEMP);
    vread(cb, bufsize, f);
    bogus=ReadCompressedLayer1(obstruct,(layer[0].sizex * layer[0].sizey), cb);
    if (bogus)
    {
        Sys_Error("LoadMAP: %s: bogus compressed obstruction data", fname);
    }
    vfree(cb);
    
    // zone grid
    vread(&bufsize, 4, f);
    cb=(char *) valloc(bufsize, "LoadMAP:cb (3)", OID_TEMP);
    vread(cb, bufsize, f);
    bogus=ReadCompressedLayer1(zone,(layer[0].sizex * layer[0].sizey), cb);
    if (bogus)
    {
        Sys_Error("LoadMAP: %s: bogus compressed zone data", fname);
    }
    vfree(cb);
    memset(zones, 0, sizeof zones);
    vread(&numzones, 4, f);
    vread(zones, numzones*50, f);
    
    //vread(&n, 1, f);
    numchrs=vgetc(f);

    vector<string_k> charlist(numchrs);

    for (i=0; i<numchrs; i++)
    {
        char c[61];
        vread(c,60,f);
        c[60]=0;
        charlist[i]=c;
    }
    
    // Cheese is good, cheese is nice. Cheese is better, than body lice.
    
    vread(&entities, 1, f);
    ents.resize(entities+1);
    for (i=0; i<entities; i++)
    {
        f >> ents[i];
    }

    // load the movescripts
    int nms=vgetc(f);
    char* ms=0;
    int msbuf[100];
    
    vread(&i, 4, f);
    vread(msbuf, nms*4, f);
    if (nms)
    {
        ms=new char[i];
        vread(ms, i, f);
    }
    
    // Compile the movescripts into something a bit sexier.
    movescripts.resize(nms);
    for (i=0; i<nms; i++)
        movescripts[i]=MoveScript(ms+msbuf[i]);

    delete[] ms;

    // set initial entity movescripts now that we actually have the scripts ready to go
    for (i=0; i<entities; i++)
        if (ents[i].movecode==Entity::mp_scripted)
            ents[i].SetMoveScript(ents[i].movescriptidx,movescripts[ents[i].movescriptidx]);
    
    vread(&i, 4, f); // # of things (unimplemented)
    LoadMapVC(f);
    vclose(f);
    
    LoadVSP(vspname);
    LoadCHRList(charlist);
    
    if (strlen(musname))
        PlayMusic(musname);

    ExecuteEvent(0);
    timer_count = 0;
}

void FreeMAP()
{
    if (obstruct)
        vfree(obstruct);
    if (zone)
        vfree(zone);
    
    for (int n=0; n<numlayers; n++)
    {
        if (layers[n]!=NULL)
        {
            vfree(layers[n]);
            layers[n]=NULL;
        }
    }
    
    playeridx=-1;
    ents.clear();
    numchrs=0;
    xwin=0;
    ywin=0;
    
    if (mapvc!=NULL)
        vfree(mapvc);

    obstruct=0;
    zone=0;
    mapvc=0;
}

void ScreenShot()
{
    class Local
    {
    public:
        static string_k SSFileName()
        {
            char fnamestr[255];

            int n=0;
            FILE* f=0;

            do
            {
                sprintf(fnamestr,"Screen %i.png",n);
                f=fopen(fnamestr,"r");
                if (f)
                    fclose(f);
                else 
                    break;

                n++;
            } while(1);

            return string_k(fnamestr);
        }
    };

    string_k fname=Local::SSFileName();

    if (gfx.bpp==1)
        WriteImage(gfx.screen,gfx.XRes(),gfx.YRes(),gfx.pal,fname.c_str());
    else
        WriteImage((u16*)gfx.screen,gfx.XRes(),gfx.YRes(),fname.c_str());

    Log::Write(va("Saved screenshot as %s",fname.c_str()));
}
//---

void ProcessControls()
{
    CheckMessages();
    
    // HookKey'd stuff
    while (char c=input.GetKey())
        if (bindarray[c]) HookKey(bindarray[c]);
        
//    if (input.key[SCAN_GRAVE])
//    {
//        console.Activate();
//    }
    
    // switching to-from windowed mode in mid-run

    // FIXME: it doesn't work. :P
    // SDL *really* doesn't like this. ;)
    /*if (input.key[SCAN_LALT] && input.key[SCAN_ENTER])
    {
        gfx.SetMode(0,0,gfx.bpp*8,!gfx.IsFullScreen());
        input.key[SCAN_ENTER]=0;
        input.key[SCAN_LALT]=0;
    }*/

    Player::HandlePlayer(ents[playeridx]);
}

void GameTick()
{
    CheckMessages();

    if (playeridx!=-1)
    {
        // when there is no player, you may not open the console or run hooked scripts. (is this correct?)
        ProcessControls();
        if (speeddemon && input.key[SCAN_LCONTROL])
            ProcessControls();
    }
    ProcessEntities();
}
