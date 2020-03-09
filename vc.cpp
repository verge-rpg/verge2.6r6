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
// |                  VergeC Interpreter  Core module                    |
// \---------------------------------------------------------------------/

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// NOTES:
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <tSB, Nov 10-ish>
// + lots of tweaking for the port to Win32.
// <vecna, aug 2>
// + changed vc var TIMER to address vctimer, not timer_count
//   timer_count is zeroed after each call to ExecuteEvent, and not zeroed
//   for hooked events.
// <aen, may 14>
// + added PaletteMorph() rgb truncation (<0 = 0, >63 = 63)
// <zero 5.8.99>
// + Mistake in PaletteMorph()? was not setting global pal2[], only a local
//   copy
// <aen, may 7>
// + added division-by-zero protection for Random()
// <aen, may 5>
// + altered vec's vc_Silhouette() to use new silhouette vdriver routines
//   instead of allocating mem, generating mask, calling sprite routines,
//   then freeing mem.
// + added vc_SilhouetteScale, vc_Tint, & vc_TintScale (unimplemented)
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//#define VC_H
#include <vector>
#include <stack>
#include <map>
#include <math.h>
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <windows.h>

#include "verge.h"
#include "misc.h"

#include "vccode.h"
#include "vcstand.h"

#include "Map/MapFile.h"
#include "vsp.h"

// prototypes
void CheckHookTimer();
void HookTimer();

// ================================= Data ====================================

int v2_touchy                           = 0;

char*   sysvc                           = 0;
char*   mapvc                           = 0;
char*   basevc                          = 0;     // VC pool ptrs
char*   code                            = 0;     // VC current instruction pointer (IP)

vector<int>     globalint;              // system.vc global int variables
int             maxint                  = 0;     // maximum allocated # of ints

vector<std::string>     
                vc_strings; // vc string workspace
int             stralloc                = 0;

int             vcreturn                = 0;     // return value of last int function
std::string     vcretstr                = "";    // return value of last string function
int             returning_type          = 0;     // hack to discern int from string returns

char*           movescriptbuf           = 0;     // VC EntityMove buffer
char            vctrack                 = 0;     // VC call tracking to verge.log

std::vector<u32>
                vcstack;    // VC stack (seperate from main stack)

int             mapevents               = 0;     // number of map events in this VC
int*            event_offsets           = 0;     // map VC offset table
// event offset marker

int             hookretrace             = 0;
int             hooktimer               = 0;
int             invc                    = 0;

char    vckill                          = 0;

// profiler
struct FuncProfile
{
    LONGLONG ticks;
    int calls;
    FuncProfile() : ticks(0), calls(0){}
};

bool    vcprofile                       = false;
LONGLONG vcpticks                       = 0;            // total ticks, so we can give percentages
std::map<std::string, FuncProfile>  funcprofile;        // ticks in each function

// FUNC/STR/VAR ARRAYS

vector<FuncDecl>        funcs;
int                     numfuncs        = 0;

vector<StrDecl> str;
int                     numstr          = 0;

vector<VarDecl> vars;
int                     numvars         = 0;

// LOCAL FUNC VARS

static int int_stack[1024 +20];
static int int_stack_base = 0, int_stack_ptr = 0;

static std::string str_stack[1024 +20];
static int str_stack_base = 0, str_stack_ptr = 0;

static int int_base[1024 +20];
static int str_base[1024 +20];
static int base_ptr = 0;

static int int_last_base = 0;
static int str_last_base = 0;


static void PushBase(int ip, int sp)
{
    if (base_ptr<0 || base_ptr>=1024)
        Sys_Error("PushBase: DEI!");
    
    int_base[base_ptr]=ip;
    str_base[base_ptr]=sp;
    
    base_ptr++;
}

static void PopBase()
{
    if (base_ptr<=0 || base_ptr>1024)
        Sys_Error("PushBase: DEI!");
    
    base_ptr--;
    
    int_stack_base = int_base[base_ptr];
    str_stack_base = str_base[base_ptr];
}

static void PushInt(int n)
{
    if (int_stack_ptr<0 || int_stack_ptr>=1024)
        Sys_Error("PushInt: DEI!");
    
    int_stack[int_stack_ptr]=n;
    
#ifdef DEBUG_LOCALS
    Log::Write(va("base=%-04d, push# %d", int_stack_base, int_stack[int_stack_ptr]));
#endif
    int_stack_ptr++;
}
static int PopInt()
{
    if (int_stack_ptr<=0 || int_stack_ptr>1024)
        Sys_Error("PopInt: DEI!");
    
    --int_stack_ptr;
#ifdef DEBUG_LOCALS
    Log::Write(va("base=%-04d, pop#  %d", int_stack_base, int_stack[int_stack_ptr]));
#endif
    
    return int_stack[int_stack_ptr];
}

static void PushStr(std::string s)
{
    if (str_stack_ptr<0 || str_stack_ptr>=1024)
        Sys_Error("PushStr: DEI!");
    
    str_stack[str_stack_ptr]=s;
    
#ifdef DEBUG_LOCALS
    Log::Write(va("base=%-04d, push$ %-60s", str_stack_base, str_stack[str_stack_ptr].c_str()));
#endif
    str_stack_ptr++;
}
static std::string PopStr()
{
    if (str_stack_ptr<=0 || str_stack_ptr>1024)
        Sys_Error("PopStr: DEI!");
    
    --str_stack_ptr;
#ifdef DEBUG_LOCALS
    Log::Write(va("base=%-04d, pop$  %-60s", str_stack_base, str_stack[str_stack_ptr].c_str()));
#endif
    
    return str_stack[str_stack_ptr];
}


// PROTOTYPES /////////////////////////////////////////////////////////////////////////////////////

std::string ResolveString();
void ExecuteSection();
void ExecuteEvent(int i);
void ExecuteUserFunc(int i);

int ProcessOperand();                 // Mutually dependant functions suck.
int ProcessIfOperand();               // Hell yeah they do, bitch.
void HandleExternFunc();
void HandleStdLib();
void ExecuteBlock();

// CODE ///////////////////////////////////////////////////////////////////////////////////////////

static int sys_codesize = 0;
static char* absolute_sys = 0;

static char xvc_sig[8] = "VERGE2X";

void LoadSystemIndex()
{
    char        buf[8];
    VFILE*      f;
    int         n;
    
    buf[0] = '\0';
    f = 0;
    
    // open system script variable/function/string offset table file
    f = vopen("system.xvc");
    if (!f)
    {
        //Sys_Error("Could not open system.idx.");
        numvars = 0;
        numfuncs = 0;
        numstr = 0;
        return;
    }
    
    vread(buf, 8, f);
    if (strncmp(buf, xvc_sig, 8))
    {
        Sys_Error("LoadSystemIndex: system.xvc contains invalid signature");
    }
    
    // skip code offset
    vread(&n, 4, f);
    
    // read # variables
    numvars = vgetq(f);
    if (numvars)
    {
        vars.resize(numvars);
        for (int i = 0; i<numvars; i++)
            vread(&vars[i], 48, f);           // TODO: annihilate this.
    }
    
    // read # functions
    numfuncs = vgetq(f);
    if (numfuncs)
    {
        funcs.resize(numfuncs);
        for (int i = 0; i<numfuncs; i++)
            vread(&funcs[i], 76, f);          // wee -_-;
    }
    
    // read # strings
    vread(&numstr, 4, f);
    if (numstr < 1)
        numstr = 1;
    str.resize(numstr);
    for (int i = 0; i<numstr; i++)
        vread(&str[i], 48, f);
    
    // done w/ this file
    vclose(f);
}

void LoadSystemCode()
{
    char        buf[8];
    VFILE*      f;
    int         code_offset;
    
    buf[0] = '\0';
    f = 0;
    code_offset = 0;
    
    // open system script code file
    f = vopen("system.xvc");
    if (!f)
    {
        //Sys_Error("Could not open system.vcs");
        sys_codesize = 0;
        sysvc = 0;
        absolute_sys = sysvc;
        numfuncs = 0;
        maxint = 0;
        stralloc = 0;
        return;
    }
    
    vread(buf, 8, f);
    if (strncmp(buf, xvc_sig, 8))
    {
        Sys_Error("LoadSystemCode: system.xvc contains invalid signature");
    }
    
    vread(&code_offset, 4, f);
    vseek(f, 0, SEEK_END);
    sys_codesize = vtell(f) - code_offset + 1;
    // see if there's actually code present
    if (sys_codesize < 1)
    {
        vclose(f);
        
        sys_codesize = 0;
        sysvc = 0;
        absolute_sys = sysvc;
        numfuncs = 0;
        maxint = 0;
        stralloc = 0;
        
        return;
    }
    // seek to code position
    vseek(f, code_offset, SEEK_SET);
    
    // grab system script code size and allocate a buffer for it
    sysvc=(char *) valloc(sys_codesize, "LoadSystemCode$sysvc", OID_VC);
    absolute_sys = sysvc;
    
    // how many funcs, global ints, and global strings?
    vread(&numfuncs, 4, f);
    vread(&maxint, 4, f);
    vread(&stralloc, 4, f);
    
    // allocate global integer and string arrays
    if (maxint)
    {
        globalint.resize(maxint);
    }
    if (stralloc)
    {
        vc_strings.resize(stralloc);
    }
    
    // read in system script code
    vread(sysvc, sys_codesize, f);
    
    vclose(f);
}

void RunSystemAutoexec()
{
    int         n;
    
    for (n = 0; n<numfuncs; n++)
    {
        std::string x = lowerCase(funcs[n].fname);
        if (x == "autoexec")
        {
            ExecuteUserFunc(n);
            return;
        }
    }

    Sys_Error("No AutoExec() found in system scripts.");
}

void LoadSystemVC()
{
    Log::Write("Initializing VC interpreter");
    
    LoadSystemIndex();
    LoadSystemCode();
    
    // initialize VC stack
    //vcstack=(u32 *)valloc(6000, "vcstack", OID_VC);
    //vcsp = vcstack;
    vcstack.reserve(1500);
    
    movescriptbuf=(char *)valloc(256*256, "movescriptbuf", OID_VC);
    
    Log::Write(va("system vclib init: %d funcs, %d ints (%d bytes), %d strings",
        numfuncs, numvars, maxint*4, numstr, stralloc));
    
    //RunSystemAutoexec();
}

void ResetVC()
{
    vcstack.clear();
    vckill = 0;
}

static int map_codesize = 0;
static char* absolute_map = 0;
void LoadMapVC(VFILE *f)
{
    //int codesize = 0;
    
    vread(&mapevents, 4, f);
    if (event_offsets)
        delete[] event_offsets;
    event_offsets = new int[mapevents];
    if (!event_offsets)
    {
        Sys_Error("LoadMapVC: memory exhausted on event_offsets");
    }
    vread(event_offsets, 4*mapevents, f);
    
    vread(&map_codesize, 4, f);
    if (map_codesize < 1)
        map_codesize = 1;
    mapvc=(char *) valloc(map_codesize, "mapvc", OID_VC);
    absolute_map = mapvc;
    vread(mapvc, map_codesize, f);
}

u8 GrabC()
{
    return *code++;
}

u16 GrabW(void)
{
    code+=2;
    return *(u16 *)(code-2);
}

u32 GrabD(void)
{
    code+=4;
    return *(u32 *)(code-4);
}

std::string GrabString()
{
    std::string ret;
    int         c;
    char        temp[32+1];     // soften the blow
    
    ret="";
    c = 0;
    while (*code)
    {
        temp[c++]=GrabC();
        if (c>=32)
        {
            c = temp[c]='\0';
            ret+=temp;
        }
    }
    if (c)
    {
        temp[c]='\0';
        ret+=temp;
    }
    code++;
    
    return ret;
}

int ReadInt(char category, int loc, int ofs)
{
    switch (category)
    {
    case op_UVAR:
        if (loc<0 || loc>=maxint)
            Sys_Error("ReadInt: bad offset to globalint (var)");
        return globalint[loc];
    case op_UVARRAY:
        if (loc<0 || loc>=maxint)
            Sys_Error("ReadInt: bad offset to globalint (arr)");
        return globalint[loc];
    case op_HVAR0:
        switch (loc)
        {
        case 0: return xwin;
        case 1: return ywin;
        case 2: return cameratracking;
        case 3: return vctimer;
        case 4: return input.up;
        case 5: return input.down;
        case 6: return input.left;
        case 7: return input.right;
        case 8: return input.b1;
        case 9: return input.b2;
        case 10: return input.b3;
        case 11: return input.b4;
        case 12: return gfx.scrx;
        case 13: return gfx.scry;
        case 14: return playeridx;
        case 15: return numentsonscreen;
        case 16: return tracker;
        case 17: return input.mousex;
        case 18: return input.mousey;
        case 19: return input.mouseb;
        case 20: return vctrack;
        case 21: return Image_Width();
        case 22: return Image_Length();
        case 23: return GetMusicVolume();
        case 24: return (int)vsp->GetTile(0);
        case 25: return 0;//lastent;
        case 26: return input.last_pressed;
        case 27: return map->Width();
        case 28: return map->Height();
        case 29: return 1;//vsync; -- vsync is always on in DirectX --tSB
        case 30: return entities;
        case 31: if (gfx.bpp==2)
                     return gfx.trans_mask;
            else
                return 0;
        case 32: if (gfx.bpp==2)
                     return 16;
            else
                return 8;

        case 33: return GetMusicPosition();
        }

        //---------------------------------------------------------------

#define RETURN_ENT_PROPERTY(idx, property)   \
    if (idx<0 || idx>=entities)             \
    {                                       \
        if (v2_touchy)                      \
            Sys_Error("Bad offset to entity."#property"[]: %i (%i total)", idx, entities);   \
        return 0;                           \
    }                                       \
    return ents[idx].property;


        case op_HVAR1:
            switch (loc)
            {
            case 0:
                if (ofs<0 || ofs>=gfx.scrx*gfx.scry)
                {
                    if (v2_touchy)
                        Sys_Error("ReadInt: bad offset to screen[]: %d (base: $%08X, %dx%d)",
                        ofs, (int) gfx.screen, gfx.scrx, gfx.scry);
                    return 0;
                }
                return gfx.bpp>1
                    ? ((unsigned short *) gfx.screen)[ofs]
                    : gfx.screen[ofs];
                return 0;
            case 1:  RETURN_ENT_PROPERTY(ofs, x)
            case 2:  RETURN_ENT_PROPERTY(ofs, y)               
            case 3:  RETURN_ENT_PROPERTY(ofs, x/16)
            case 4:  RETURN_ENT_PROPERTY(ofs, y/16)
            case 5:  RETURN_ENT_PROPERTY(ofs, direction)
            case 6:  RETURN_ENT_PROPERTY(ofs, ismoving)
            case 7:  RETURN_ENT_PROPERTY(ofs, specframe)
            case 8:  RETURN_ENT_PROPERTY(ofs, speed)
            case 9:  RETURN_ENT_PROPERTY(ofs, movecode)
            case 10:
                if (ofs<0 || ofs>=entities)
                {
                    if (v2_touchy)
                        Sys_Error("ReadInt: bad offset to entidx[]: %d (%d total)",
                        ofs, entities);
                    return 0;
                }
                return entidx[ofs];
            case 11:
                if (ofs<0 || ofs>=128)
                {
                    if (v2_touchy)
                        Sys_Error("ReadInt: bad offset to key[]: %d", ofs);
                    return 0;
                }
                return input.key[ofs];  //scantokey[ofs]];
            case 12:
                if (ofs<0 || ofs>=map->NumLayers())
                {
                    if (v2_touchy)
                        Sys_Error("ReadInt: bad offset to layer.hline[]: %d (%d total)",
                        ofs, map->NumLayers());
                    return 0;
                }
                return 0;//layer[ofs].hline;
                
            case 13: return (int) (*(u8 *)ofs);
            case 14: return (int) (*(u16 *)ofs);
            case 15: return (int) (*(u32 *)ofs);
                
            case 16:
                if (ofs<0 || ofs>=3*256)
                {
                    if (v2_touchy)
                        Sys_Error("ReadInt: bad offset to pal[]: %d", ofs);
                    return 0;
                }
                return (int) gfx.pal[ofs];
                
            case 17: return (int) (*(char *)ofs);
            case 18: return (int) (*(short*)ofs);
            case 19: return (int) (*(int  *)ofs);

            case 20: RETURN_ENT_PROPERTY(ofs, isobs)
            case 21: RETURN_ENT_PROPERTY(ofs, canobs)
            case 22: RETURN_ENT_PROPERTY(ofs, autoface)
            case 23: RETURN_ENT_PROPERTY(ofs, visible)
            case 24: RETURN_ENT_PROPERTY(ofs, on)

        /*case 25: // I hate this hacked pointer crap --tSB
            return (int)chr[ents[ofs].chrindex]->GetFrame(0); // chr_data
        case 26:return chr[ents[ofs].chrindex]->Width();      // entity.width
        case 27:return chr[ents[ofs].chrindex]->Height();     // entity.height
        case 28:return ents[ofs].chrindex;                    // entity.chrindex
        */
}            
        case op_LVAR:
            if (loc<0 || loc>19)
            {
                Sys_Error("ReadInt: bad offset to local ints: %d", loc);
            }
#ifdef DEBUG_LOCALS
            Log::Write(va("op_LVAR: int_stack_base=%d, loc=%d", int_stack_base, loc));
#endif
            return int_stack[int_stack_base+loc];
            
        default:
            Sys_Error("VC Execution error: Invalid ReadInt category %d", (int) category);
        }
        
        return 0;

#undef RETURN_ENT_PROPERTY
}

void WriteInt(char category, int loc, int ofs, int value)
{
    switch (category)
    {
    case op_UVAR:
        if (loc<0 || loc>=maxint)
        {
            if (v2_touchy)
                Sys_Error("WriteInt: bad offset to globalint (var)");
            break;
        }
        globalint[loc]=value;
        break;
    case op_UVARRAY:
        if (loc<0 || loc>=maxint)
        {
            if (v2_touchy)
                Sys_Error("WriteInt: bad offset to globalint (arr)");
            break;
        }
        globalint[loc]=value;
        break;
    case op_HVAR0:
        switch (loc)
        {
        case 0:  xwin = value;                  return;
        case 1:  ywin = value;                  return;
        case 2:  cameratracking=(u8)value;      return;
        case 3:  vctimer = value;               return;
        case 4:  input.up = value != 0;         return;
        case 5:  input.down = value != 0;       return;
        case 6:  input.left = value != 0;       return;
        case 7:  input.right = value != 0;      return;
        case 8:  input.b1 = value != 0;         return;
        case 9:  input.b2 = value != 0;         return;
        case 10: input.b3 = value != 0;         return;
        case 11: input.b4 = value != 0;         return;
        case 16: tracker=(u8)value;             return;
        case 17: input.mousex = value;          return;
        case 18: input.mousey = value;          return;
        case 19: input.mouseb = value;          return;
        case 20: vctrack=(char)value;           return;
        case 23: SetMusicVolume(value);         return;
        case 26: input.last_pressed = value;    return;
        case 29: return; // vsync = value; return;

        case 33: SetMusicPosition(value);       return;

        }

#define ASSIGN_ENT_PROPERTY(idx, property, type)   \
    if (idx<0 || idx>=entities)             \
    {                                       \
        if (v2_touchy)                      \
            Sys_Error("WriteInt: bad offset to entity."#property"[]: %i (%i total)", idx, entities);  \
        return;                             \
    }                                       \
    ents[idx].property=(type)value;         \
    return;

#define ASSIGN_BOOL_ENT_PROPERTY(idx, property)  \
    if (idx<0 || idx>=entities)                 \
    {                                           \
        if (v2_touchy)                          \
            Sys_Error("WriteInt: bad offset to entity."#property"[]: %i (%i total)", idx, entities);  \
        return;                                 \
    }                                           \
    ents[idx].property = value != 0;            \
    return;

    case op_HVAR1:
            switch (loc)
            {
            case 0:
                if (ofs < 0 || ofs >= gfx.scrx*gfx.scry)
                {
                    if (v2_touchy)
                        Sys_Error("WriteInt: bad offset to screen[]: %d (dest: $%08X, %dx%d)",
                        ofs, (int) gfx.screen, gfx.scrx, gfx.scry);
                    return;
                }
                if (gfx.bpp>1)
                    ((u16*)gfx.screen)[ofs]=(u16) value;
                else
                    gfx.screen[ofs] = (u8) value;
                return;
            case 1:  ASSIGN_ENT_PROPERTY(ofs, x, int)
            case 2:  ASSIGN_ENT_PROPERTY(ofs, y, int)

            case 3:
                if (ofs < 0 || ofs >= entities)
                {
                    if (v2_touchy)
                        Sys_Error("WriteInt: bad offset to entity.tx[]: %d (%d total)",
                        ofs, entities);
                    return;
                }
                ents[ofs].x = value*16;
                return;
            case 4:
                if (ofs < 0 || ofs >= entities)
                {
                    if (v2_touchy)
                        Sys_Error("WriteInt: bad offset to entity.ty[]: %d (%d total)",
                        ofs, entities);
                    return;
                }
                ents[ofs].y = value*16;
                return;
            case 5: ASSIGN_ENT_PROPERTY(ofs, direction, Direction)
            case 6: Sys_Error("WriteInt: entity.moving[] is read-only.");
            case 7: ASSIGN_ENT_PROPERTY(ofs, specframe, int)
            case 8: ASSIGN_ENT_PROPERTY(ofs, speed, int)
            case 9: ASSIGN_ENT_PROPERTY(ofs, movecode, Entity::MovePattern)
            case 10: //???

            case 11:
                if (ofs < 0 || ofs >= 128)
                {
                    if (v2_touchy)
                        Sys_Error("WriteInt: bad offset to key[]: %d (%d total)",
                        ofs, entities);
                    return;
                }
                input.key[ofs] = value != 0;
                return;
            case 12:
                if (ofs < 0 || ofs >= map->NumLayers())
                {
                    if (v2_touchy)
                        Sys_Error("WriteInt: bad offset to layer.hline[]: %d (%d total)", ofs, map->NumLayers());
                    return;
                }
                //layer[ofs].hline = (unsigned char) value;
                return;
                
            case 13: (*(u8 *)ofs)=(u8) value; return;
            case 14: (*(u16 *)ofs)=(u16) value; return;
            case 15: (*(u32 *)ofs)=(u32) value; return;
                
            case 16:
                if (ofs < 0 || ofs >= 3*256)
                {
                    if (v2_touchy)
                        Sys_Error("WriteInt: bad offset to gfx.pal[]: %d", ofs);
                    return;
                }
                gfx.pal[ofs] = (u8) value;
                return;
                
            case 17: (*(u8 *)ofs)=(u8) value; return;
            case 18: (*(u16*)ofs)=(u16) value; return;
            case 19: (*(u32*)ofs)=(u32) value; return;
                
            case 20: ASSIGN_BOOL_ENT_PROPERTY(ofs, isobs)
            case 21: ASSIGN_BOOL_ENT_PROPERTY(ofs, canobs)
            case 22: ASSIGN_BOOL_ENT_PROPERTY(ofs, autoface)
            case 23: ASSIGN_BOOL_ENT_PROPERTY(ofs, visible)
            case 24: ASSIGN_BOOL_ENT_PROPERTY(ofs, on)
        }
        case op_LVAR:
            if (loc<0 || loc>19)
            {
                Sys_Error("WriteInt: bad offset to local ints: %d", loc);
            }
            int_stack[int_stack_base+loc]=value;
            return;
            
        default:
            Sys_Error("VC Execution error: Invalid WriteInt category %d", (int) category);
        }

#undef ASSIGN_ENT_PROPERTY
#undef ASSIGN_BOOL_ENT_PROPERTY
}

int ResolveOperand()
{
    int cr = 0;
    int d = 0;
    u8 c = 0;
    
    cr = ProcessOperand();      // Get base number
    while (1)
    {
        c = GrabC();
        switch (c)
        {
        case op_ADD: cr += ProcessOperand(); continue;
        case op_SUB: cr -= ProcessOperand(); continue;
        case op_DIV:
            d = ProcessOperand();
            if (!d) cr = 0; else cr /= d;
            continue;
        case op_MULT: cr = cr * ProcessOperand(); continue;
        case op_MOD:
            d = ProcessOperand();
            if (!d) cr = 0; else cr %= d;
            continue;
        case op_SHL: cr = cr << ProcessOperand(); continue;
        case op_SHR: cr = cr >> ProcessOperand(); continue;
        case op_AND: cr = cr & ProcessOperand(); continue;
        case op_OR:  cr = cr | ProcessOperand(); continue;
        case op_XOR: cr = cr ^ ProcessOperand(); continue;
        case op_END: break;
        }
        break;
    }
    return cr;
}

int ProcessOperand()
{
    u8 op_desc = 0;
    u8 c = 0;
    u32 d = 0;
    u32 ofs = 0;
    
    op_desc = GrabC();
    switch (op_desc)
    {
    case op_IMMEDIATE: return GrabD();
    case op_HVAR0: c = GrabC(); return ReadInt(op_HVAR0, c, 0);
    case op_HVAR1: c = GrabC(); ofs = ResolveOperand(); return ReadInt(op_HVAR1, c, ofs);
    case op_UVAR:  d = GrabD(); return ReadInt(op_UVAR, d, 0);
    case op_UVARRAY: d = GrabD(); d+=ResolveOperand(); return ReadInt(op_UVARRAY, d, 0);
    case op_LVAR:
        c = GrabC();
        if (c>19)
        {
            Sys_Error("ProcessOperand: bad offset to local ints: %d", c);
        }
        return int_stack[int_stack_base+c];
    case op_BFUNC:
        HandleStdLib();
        return vcreturn;
    case op_UFUNC:
        HandleExternFunc();
        return vcreturn;
    case op_GROUP: return ResolveOperand();
    default:
        Sys_Error("VC Execution error: Invalid operand %d.", op_desc);
        break;
    }
    return 0;
}

std::string HandleStringOperand()
{
    std::string ret;
    int         c;
    
    c = GrabC();
    switch (c)
    {
    case s_IMMEDIATE:
        ret = GrabString();
        break;
        
    case s_GLOBAL:
        c = GrabW();
        if (c>=0 && c<stralloc)
        {
            ret = vc_strings[c];
        }
        else
            Sys_Error("HandleStringOperand: bad offset to vc_strings");
        break;
        
    case s_ARRAY:
        c = GrabW();
        c+=ResolveOperand();
        if (c>=0 || c<stralloc)
        {
            ret = vc_strings[c];
        }
        else
            Sys_Error("HandleStringOperand: bad offset to vc_strings");
        break;
        
    case s_NUMSTR:
        ret = va("%d", ResolveOperand());
        break;
        
    case s_LEFT:
        ret = ResolveString();
        c = ResolveOperand();
        if (c < ret.length())
            ret = ret.substr(0, c);
        break;
        
    case s_RIGHT:
        ret = ResolveString();
        c = ret.length() - ResolveOperand();
        if (c < ret.length())
            ret = ret.substr(c);
        else
            ret.clear();
        break;
        
    case s_MID:
        {
            ret = ResolveString();
            int start = ResolveOperand();
            int len = ResolveOperand();

            if (start >= ret.length())
                ret = "";
            else if (start + len >= ret.length())
                ret = ret.substr(start);
            else
                ret = ret.substr(start, len);
        }
        break;
        
    case s_CHR:
        ret=(char)ResolveOperand();
        break;
        
    case s_LOCAL:
        c = GrabC();
#ifdef DEBUG_LOCALS
        Log::Write(va("s_LOCAL: str_stack_base=%d, c=%d", str_stack_base, c));
#endif
        if (c>=0 && c<20)
        {
            ret = str_stack[str_stack_base+c];
        }
        else
            Sys_Error("HandleStringOperand: bad offset to local strings: %d", c);
        break;
        
        // sweet
    case s_UFUNC:
        HandleExternFunc();
        ret = vcretstr;
        break;
        
    default:
        Sys_Error("Invalid VC string operand %d", c);
    }
    
    return ret;
}

std::string ResolveString()
{
    std::string ret;
    int         c;
    
    ret = HandleStringOperand();
    do
    {
        c = GrabC();
        if (s_ADD==c)
            ret+=HandleStringOperand();
        else if (s_END!=c)
            Sys_Error("VC execution error: Unknown string operator %d", c);
    } while (c!=s_END);
    
    return ret;
}

void vcpush(u32 info)
{
    if (vcstack.size()>1500)//vcsp >= vcstack+1500)
        Sys_Error("VC stack overflow.");
    
    //*vcsp++ = info;
    vcstack.push_back(info);
}

u32 vcpop()
{
    if (vcstack.size()==0)//vcsp <= vcstack)
        Sys_Error("VC stack underflow.");
    
    //return *--vcsp;
    u32 val = vcstack.back();
    vcstack.pop_back();
    return val;
}

// This might be better place in conlib or something, I dunno. --tSB
//extern std::string Con_GetArg(unsigned int x);
//std::string Con_GetArg(int x) { return ""; }

void  ReadVCVar()
{
    /*int i = 0;
    int j = 0;
    int ofs;
    
    std::string arg1 = Con_GetArg(1).lower();
    
    // Search the int list
    for (i = 0; i<=numvars; i++)
        if (!strcmp(vars[i].vname, arg1.c_str()))
            break;
        
        if (i<numvars)
        {
            j = vars[i].varstartofs;
            
            if (vars[i].arraylen>1)                            // if it's an array
            {
                ofs = atoi(Con_GetArg(2).c_str());               // get the next argument, and use it as an offset
                j = globalint[j+ofs];
//                sprintf(strbuf, "%s[%d]=%d", vars[i].vname, ofs, j);
            }
            else
            {
                j = globalint[j];
  //              sprintf(strbuf, "%s=%d", vars[i].vname, j);
            }
//            Console_Printf(strbuf);
            return;
        }
        
        /*  // not an int.  Check the string variables.
        for (i = 0; i<=numstr; i++)
        if (!strcmp(str[i].vname, (const char*)arg1))
        break;
        
          if (i<numstr)
          {
          if (str[i].arraylen>1) // array?
          {
          ofs = atoi((const char*)Con_GetArg(2)); // get the offset
          sprintf(strbuf, "%s[%d]=%s", str[i].vname, ofs, (const char*)vc_strings[i+ofs]);
          }
          else
          {
          sprintf(strbuf, "%s=%s", str[i].vname, (const char*)vc_strings[i]);
          }
          Console_Printf(strbuf);
          return;
}*/
  //      Console_Printf("No such VC variable.");       */
}

void WriteVCVar()
{
    /*int i = 0;
    int j = 0;
    int ofs;
    
    std::string arg1 = Con_GetArg(1).lower();
    
    for (i = 0; i<=numvars; i++)
        if (!strcmp(vars[i].vname, arg1.c_str()))
            break;
        
        if (i<numvars)
        {
            j = vars[i].varstartofs;
            if (vars[i].arraylen>1)
            {
                ofs = atoi(Con_GetArg(2).c_str());
                globalint[j+ofs]=atoi(Con_GetArg(3).c_str());
//                sprintf(strbuf, "%s[%d]=%d", vars[i].vname, ofs, atoi(Con_GetArg(3).c_str()));
            }
            else 
            {
                globalint[j]=atoi(Con_GetArg(2).c_str());
  //              sprintf(strbuf, "%s=%d", vars[i].vname, atoi(Con_GetArg(2).c_str()));
            }
//            Console_Printf(strbuf);
            return;
        }
        /*  for (i = 0; i<=numstr; i++)
        if (!strcmp(str[i].vname, (const char*)arg1))
        break;
        
          if (i<numstr)
          {
          j=(int) stringbuf + (i*256);
          strncpy((char *)j, (const char*)Con_GetArg(2), 255);
          sprintf(strbuf, "%s:%s", str[i].vname, (const char*)Con_GetArg(2));
          Console_Printf(strbuf);
          return;
}*/
  //      Console_Printf("No such VC variable.");
        
}


void DumpProfile()
{
    if (!vcprofile)
        return;

    std::ofstream f("vcprofile.htm");
    f << "Total ticks " << vcpticks << "<p>" << std::endl;
    f << "<table><tr><td>Function</td><td>Total time</td><td>%</td><td>Times called</td></tr>"<< std::endl;
    for (std::map<std::string, FuncProfile>::iterator i = funcprofile.begin(); i!=funcprofile.end(); i++)
    {
        FuncProfile& F = i->second;
        int p = int(F.ticks*10000/vcpticks);
        f << "<tr><td>" << i->first.c_str() << "</td><td>" << F.ticks << "</td><td>" << (float)(1.0f*p/100) << "</td><td>" << F.calls << "</td></tr>" << std::endl;
    }
    f << "</table>" << std::endl;
}

// ===================== End VC Standard Function Library =====================

void HandleStdLib()
{
    u8 c = GrabC();

    if (c<=0 || c>123)
        Sys_Error("VC Execution error: Invalid STDLIB index. (%d)", (int)c);

    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);//SDL_GetTicks();
    vcfunctions[c]();

    if (vcprofile)
    {
        std::stringstream str;
        str << "StdFunc" << (int)c;

        LARGE_INTEGER u;
        QueryPerformanceCounter(&u);
        FuncProfile& f = funcprofile[str.str().c_str()];
        
        f.ticks+=u.QuadPart-t.QuadPart;
        f.calls++;
        vcpticks+=u.QuadPart-t.QuadPart;
    }
}

// ========================== VC Interpretation Core ==========================

int ProcessIf()
{
    u8 exec, c;
    
    exec=(u8)ProcessIfOperand();        // Get base value;
    while (1)
    {
        c = GrabC();
        switch (c)
        {
        case i_AND: exec=(u8)(exec & ProcessIfOperand()); continue;
        case i_OR: exec=(u8)(exec | ProcessIfOperand()); continue;
        case i_UNGROUP: break;
        }
        break;
    }
    return exec;
}

int ProcessIfOperand()
{
    u8 op_desc;
    int eval;
    
    eval = ResolveOperand();
    op_desc = GrabC();
    switch (op_desc)
    {
    case i_ZERO: if (!eval) return 1; else return 0;
    case i_NONZERO: if (eval) return 1; else return 0;
    case i_EQUALTO: if (eval == ResolveOperand()) return 1; else return 0;
    case i_NOTEQUAL: if (eval != ResolveOperand()) return 1; else return 0;
    case i_GREATERTHAN: if (eval > ResolveOperand()) return 1; else return 0;
    case i_GREATERTHANOREQUAL: if (eval >= ResolveOperand()) return 1; else return 0;
    case i_LESSTHAN: if (eval < ResolveOperand()) return 1; else return 0;
    case i_LESSTHANOREQUAL: if (eval <= ResolveOperand()) return 1; else return 0;
    case i_GROUP: if (ProcessIf()) return 1; else return 0;
    }
    
    return 0;
}

void HandleIf()
{
    char *d;
    
    if (ProcessIf())
    {
        GrabD();
        return;
    }
    d           =(char *)GrabD();
    code        =(char *)(int)basevc+(int)d;
}

static int routine_depth = 0;

void HandleExternFunc()
{
    int         n;
    int ilb = 0, slb = 0;
    
    n = GrabW();
    if (n<0 || n>=numfuncs)
    {
        Sys_Error("HandleExternFunc: VC sys script out of bounds (%d/%d)", n, numfuncs);
    }
    FuncDecl& func = funcs[n];
    
    ilb = int_last_base;
    slb = str_last_base;
    PushBase(int_last_base, str_last_base);
    
#ifdef DEBUG_LOCALS
    Log::Write(">>> HandleExternFunc");
#endif
    int         isp, ssp;
    
    // we do not set the new base until we're done reading in the arguments--
    // this is because we might still need to read in local vars passed from the
    // previous function (lookup for locals works off current base values).
    // for now, just tag the to-be bases.
    isp =int_stack_ptr;
    ssp =str_stack_ptr;
    // allocate stack space
    if (func.numlocals)
    {
        // read in arguments
        for (n = 0; n<func.numargs; n++)
        {
            switch (func.argtype[n])
            {
            case 1:
                PushInt(ResolveOperand());
                break;
            case 3:
                PushStr(ResolveString());
                break;
            }
        }
        // finish off allocating locals
        while (n<func.numlocals)
        {
            switch (func.argtype[n])
            {
            case 1:
                PushInt(0);
                break;
            case 3:
                PushStr("");
                break;
            }
            n++;
        }
    }
    // now we're safe to set the bases
    int_stack_base = int_last_base = isp;
    str_stack_base = str_last_base = ssp;
    
    vcpush((u32)basevc);
    vcpush((u32)code);
    
    basevc      =sysvc;
    code        =(char *)(basevc + func.syscodeofs);

    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    
    if (vctrack)
    {
        for (n = 0; n<routine_depth; n++)
            Log::Writen("  ");
        Log::Write(va(" --> %s",
            func.fname));
        routine_depth++;
    }
    
    ExecuteBlock();
    
    basevc      =(char *)vcpop();
    
    // restore previous base
    PopBase();
    int_last_base = ilb;
    str_last_base = slb;
    // free stack space
    if (func.numlocals)
    {
        // clear out all locals (args + 'true' locals)
        for (n = 0; n<func.numlocals; n++)
        {
            switch (func.argtype[n])
            {
            case 1:
                PopInt();
                break;
            case 3:
                PopStr();
                break;
            }
        }
    }
#ifdef DEBUG_LOCALS
    Log::Write("<<< HandleExternFunc");
#endif
    
    if (vctrack)
    {
        routine_depth--;
        for (n = 0; n<routine_depth; n++)
            Log::Writen("  ");
        Log::Write(va(" <-- %s", func.fname));
    }

    if (vcprofile)
    {
        LARGE_INTEGER u;
        QueryPerformanceCounter(&u);
        FuncProfile& f = funcprofile[func.fname];
        f.ticks+=u.QuadPart-t.QuadPart;
        f.calls++;
        vcpticks+=u.QuadPart-t.QuadPart;
    }
}

void HandleAssign()
{
    int         op, c, base, offset, value;
    
    c = GrabC();
    
    // string assignment
    if (c == op_STRING)
    {
        offset = GrabW();
        c = GrabC();
        if (c != a_SET)
        {
            Sys_Error("VC execution error: Corrupt string assignment");
        }
        if (offset>=0 && offset<stralloc)
        {
            vc_strings[offset]=ResolveString();
        }
        else
            Sys_Error("HandleAssign: bad offset to vc_strings (var)");
        return;
    }
    // string array assignment
    if (c == op_SARRAY)
    {
        offset = GrabW();
        offset+=ResolveOperand();
        c = GrabC();
        if (c != a_SET)
        {
            Sys_Error("VC execution error: Corrupt string assignment");
        }
        if (offset>=0 && offset<stralloc)
        {
            vc_strings[offset]=ResolveString();
        }
        else
            Sys_Error("HandleAssign: bad offset to vc_strings (arr)");
        return;
    }
    // local string assignment
    if (c == op_SLOCAL)
    {
        offset = GrabW();
        c = GrabC();
        if (c != a_SET)
        {
            Sys_Error("VC execution error: Corrupt string assignment");
        }
        if (offset>=0 && offset<20) //MAX_LOCAL_STRINGS)
        {
            str_stack[str_stack_base+offset]=ResolveString();
        }
        else
            Sys_Error("HandleAssign: bad offset to local strings: %d", c);
        return;
    }
    
    // integer assignment
    base = offset = 0;
    switch (c)
    {
    case op_UVAR:               base = GrabD(); break;
    case op_UVARRAY:    base = GrabD(); base+=ResolveOperand(); break;
    case op_HVAR0:              base = GrabC(); break;
    case op_HVAR1:              base = GrabC(); offset = ResolveOperand(); break;
    case op_LVAR:               base = GrabC(); break;
        
    default:
        Sys_Error("VC Execution error: Unknown assignment category.");
    }
    value = ReadInt((char)c, base, offset);
    op = GrabC();
    switch(op)
    {
    case a_SET:         value = ResolveOperand(); break;
    case a_INC:         value++; break;
    case a_DEC:         value--; break;
    case a_INCSET:      value+=ResolveOperand(); break;
    case a_DECSET:      value-=ResolveOperand(); break;
        
    default:
        Sys_Error("VC Execution error: Invalid assignment operator %d.", op);
    }
    WriteInt((char)c, base, offset, value);
}

void HandleSwitch()
{
    int realvalue = 0;
    int compvalue = 0;
    u8 c = 0;
    u8 *next = 0;
    
    realvalue = ResolveOperand();
    c = GrabC();
    while (c != opRETURN)
    {
        compvalue = ResolveOperand();
        next=(u8 *)GrabD();
        if (compvalue != realvalue)
        {
            code=(char *)(int)basevc+(int)next;
            c = GrabC();
            continue;
        }
        ExecuteSection();
        c = GrabC();
    }
}

void ExecuteVC()
{
    u8 c = 0;
    
    while (1)
    {
        if (vckill) break;
        CheckMessages();
        if (input.key[SCAN_LMENU] && input.key['x']) Sys_Error("");
        
        c = GrabC();
        switch (c)
        {
        case opEXEC_STDLIB: HandleStdLib(); break;
        case opEXEC_LOCALFUNC: break;
        case opEXEC_EXTERNFUNC: HandleExternFunc(); break;
        case opIF: HandleIf(); break;
        case opELSE: break;
        case opGOTO: code = basevc+GrabD(); break;
        case opSWITCH: HandleSwitch(); break;
        case opASSIGN: HandleAssign(); break;
        case opRETURN: code=(char *) vcpop(); break;
        case opSETRETVAL: vcreturn = ResolveOperand(); break;
        case opSETRETSTRING: vcretstr = ResolveString(); break;
            
        default:
            Sys_Error("Internal VC execution error. (%d)", (int) code - (int) basevc);
        }
        
        if ((int)code != -1)
            continue;
        else
            break;
    }
}

void ExecuteBlock()
{
    u8 c = 0;
    int count = 500;
    
    while (1)
    {
        if (vckill) break;
        if (!--count)
        {
            CheckMessages();
            count = 500;
        }
        if (input.key[SCAN_LMENU] && input.key[SCAN_X]) Sys_Error("");
        
        c = GrabC();
        switch (c)
        {
        case opEXEC_STDLIB:     HandleStdLib();            break;
        case opEXEC_LOCALFUNC:                             break;
        case opEXEC_EXTERNFUNC: HandleExternFunc();        break;
        case opIF:              HandleIf();                break;
        case opELSE:                                       break;
        case opGOTO:            code = basevc+GrabD();       break;
        case opSWITCH:          HandleSwitch();            break;
        case opASSIGN:          HandleAssign();            break;
        case opRETURN:          code=(char *) vcpop();     break;
        case opSETRETVAL:       vcreturn = ResolveOperand(); break;
        case opSETRETSTRING:    vcretstr = ResolveString();  break;
            
        default:
            Sys_Error("Internal VC execution error. (%d)", (int) code - (int) basevc);
        }
        
        /*              if (c != opRETURN)
        continue;
        else
        break;*/
        if (c == opRETURN)
            break;
    }
}

void ExecuteSection()
{
    u8 c = 0;
    
    while (1)
    {
        if (vckill) break;

        CheckMessages();
        if (input.key[SCAN_LMENU] && input.key[SCAN_X]) Sys_Error("");
        
        c = GrabC();
        switch (c)
        {
        case opEXEC_STDLIB: HandleStdLib(); break;
        case opEXEC_LOCALFUNC: break;
        case opEXEC_EXTERNFUNC: HandleExternFunc(); break;
        case opIF: HandleIf(); break;
        case opELSE: break;
        case opGOTO: code = basevc+GrabD(); break;
        case opSWITCH: HandleSwitch(); break;
        case opASSIGN: HandleAssign(); break;
        case opRETURN: break;
        case opSETRETVAL: vcreturn = ResolveOperand(); break;
        case opSETRETSTRING: vcretstr = ResolveString(); break;
        default:
            Sys_Error("Internal VC execution error. (%d)", (int) code - (int) basevc);
        }
        
        if (c != opRETURN)
            continue;
        else
            break;
    }
}

void ExecuteEvent(int ev)
{
    if (ev<0 || ev>=mapevents)
    {
        Sys_Error("ExecuteEvent: VC event out of bounds (%d)", ev);
    }
    
    ++invc;
    
    vcpush((u32)code);
    vcpush((u32)basevc);
    
    basevc      =mapvc;
    code        =basevc+event_offsets[ev];
    
    vcpush ((u32)-1);
    ExecuteVC();
    
    basevc      =(char *)vcpop();
    code        =(char *)vcpop();
    
    --invc;
    
    //timer_count = 0;
}

void ExecuteUserFunc(int ufunc)
{
    int ilb = 0, slb = 0;
    
    if (ufunc<0 || ufunc>=numfuncs)
    {
        Sys_Error("VC sys script out of bounds (%d)", ufunc);
    }
    FuncDecl& func = funcs[ufunc];
    
#ifdef DEBUG_LOCALS
    Log::Write(">>> ExecuteUserFunc");
#endif
    
    // straight push of the current stack pointers
    ilb = int_last_base; slb = str_last_base;
    PushBase(int_last_base, str_last_base);
    int_stack_base = int_last_base = int_stack_ptr;
    str_stack_base = str_last_base = str_stack_ptr;
    int         n;
    // allocate stack space
    if (func.numlocals)
    {
        // only locals
        for (n = 0; n<func.numlocals; n++)
        {
            switch (func.argtype[n])
            {
            case 1:
                PushInt(0);
                break;
            case 3:
                PushStr("");
                break;
            }
        }
    }
    
    vcpush((u32)code);
    vcpush((u32)basevc);
    
    basevc      =sysvc;
    code        =(char *)(basevc + func.syscodeofs);
    
    vcpush((u32)-1);
    
    ExecuteVC();
    
    basevc      =(char *)vcpop();
    code        =(char *)vcpop();
    
    PopBase();
    int_last_base = ilb;
    str_last_base = slb;
    // free stack space
    if (func.numlocals)
    {
        // clear out all locals (args + 'true' locals)
        for (n = 0; n<func.numlocals; n++)
        {
            switch (func.argtype[n])
            {
            case 1:
                PopInt();
                break;
            case 3:
                PopStr();
                break;
            }
        }
    }
#ifdef DEBUG_LOCALS
    Log::Write("<<< ExecuteUserFunc");
#endif
}

void HookRetrace()
{
    if (!hookretrace) return;
    
    if (hookretrace <  USERFUNC_MARKER)
        ExecuteEvent(hookretrace);
    if (hookretrace >= USERFUNC_MARKER)
        ExecuteUserFunc(hookretrace-USERFUNC_MARKER);
}

void CheckHookTimer()
{
    while (hktimer)
    {
        HookTimer();
        hktimer--;
    }
}

void HookTimer()
{
    if (!hooktimer) return;
    
    if (hooktimer <  USERFUNC_MARKER)
        ExecuteEvent(hooktimer);
    if (hooktimer >= USERFUNC_MARKER)
        ExecuteUserFunc(hooktimer-USERFUNC_MARKER);
}

void HookKey(int script)
{
    if (!script) return;
    
    if (script <  USERFUNC_MARKER)
        ExecuteEvent(script);
    if (script >= USERFUNC_MARKER)
        ExecuteUserFunc(script-USERFUNC_MARKER);

    timer_count = 0;
}
