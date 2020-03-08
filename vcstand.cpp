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

#include "vcstand.h"
#include "vc.h"
#include "vfile.h"
#include "engine.h"
#include "misc.h"
#include "vccode.h"
#include "sincos.h"
#include <math.h>

extern int ResolveOperand();    // vc.cpp
extern string_k ResolveString();// ditto
extern int vcreturn;

extern u8 GrabC();
extern u32 GrabD();
extern u16 GrabW();

extern string_k* vc_strings;
extern int* globalint;
extern int maxint;

int lucentmode=0;

void vc_UnPress()
{
    int		n;
    n=ResolveOperand();
    
    input.UnPress(n);
}

void vc_Exit_()
{
    string_k	message;
    
    message=ResolveString();
    
    Sys_Error(message.c_str());
}

void vc_Message()
{
    string_k	message;
    int		n;
    
    message=ResolveString();
    n=ResolveOperand();
    
    Message_Send(message, n);
}

void vc_Malloc()
{
    int		n;
    
    n=ResolveOperand();
    
    vcreturn=(int)valloc(n, "vcreturn", OID_VC);
    
    Log::Write(va("VC allocating %u bytes, ptr at 0x%08X.", n, vcreturn));
    
    if (!vcreturn)
        Message_Send("Warning: VC failed malloc", 750);
}

void vc_Free()
{
    int		ptr;
    
    ptr=ResolveOperand();
    
    vfree((void *)ptr);
    
    Log::Write(va("VC freeing allocated heap at 0x%08X.", ptr));
}

void vc_pow()
{
    int		a, b;
    
    a=ResolveOperand();
    b=ResolveOperand();
    
    vcreturn=(int)pow(a, b);
}

void vc_LoadImage()
{
    string_k	filename;
    
    filename=ResolveString();
    
    vcreturn=(int)Image_LoadBuf(filename.c_str());
    if (!vcreturn)
    {
        Sys_Error("vc_loadimage: %s: unable to open", filename.c_str());
    }
}

void vc_CopySprite()
{
    int		x, y, width, length;
    int		ptr;
    
    x		=ResolveOperand();
    y		=ResolveOperand();
    width	=ResolveOperand();
    length	=ResolveOperand();
    ptr		=ResolveOperand();
    
    if (lucentmode)
        gfx.CopySpriteLucent(x, y, width, length, (u8 *)ptr,lucentmode);
    else
        gfx.CopySprite(x, y, width, length, (u8 *)ptr);
}

void vc_TCopySprite()
{
    int		x, y, width, length;
    int		ptr;
    
    x		=ResolveOperand();
    y		=ResolveOperand();
    width	=ResolveOperand();
    length	=ResolveOperand();
    ptr		=ResolveOperand();
    
    if (lucentmode)
        gfx.TCopySpriteLucent(x, y, width, length, (u8 *)ptr,lucentmode);
    else
        gfx.TCopySprite(x, y, width, length, (u8 *)ptr);
}

void vc_ShowPage()
{
    gfx.ShowPage();
}

void vc_EntitySpawn()
{
    int		tilex, tiley;
    string_k	chr_filename;
    
    tilex=ResolveOperand();
    tiley=ResolveOperand();
    chr_filename=ResolveString();
    
    vcreturn=AllocateEntity(tilex, tiley, chr_filename.c_str());
}

void vc_SetPlayer()
{
    int		n;
    
    n=ResolveOperand();
    if (n<0 || n>=entities)
    {
        Sys_Error("vc_SetPlayer: entity index out of range (attempted %d)", n);
    }
    
    playeridx=n;
//    player=&ents[n];
//    playernum=(u8)n;
    
    ents[n].Stop();
}

void vc_Map()
{
    hookretrace=0;
    hooktimer=0;
    vckill=1;
    
    startmap=ResolveString();
}

void vc_LoadFont()
{
    string_k	filename;
    
    filename=ResolveString();
    
    vcreturn=font.Load(filename.c_str());
}

void vc_PlayFLI()
{
    Log::Write("vc_PlayFLI disabled.");
    
    ResolveString();	// FLI filename
/*    
    string_k fli_filename=ResolveString();
    
    BITMAP flibuf;
    flibuf.w=screen_width;
    flibuf.h=screen_length;
    flibuf.data=screen;
    
    VFILE* f=vopen((const char*)fli_filename);
    if (!f)
    {
        Sys_Error("vc_PlayFLI: could not open %s.", (const char*)fli_filename);
    }
    
    unsigned int n=filesize(f);
    u8* data=(u8 *)valloc(n, "vc_PlayFLI:data", 0);
    if (!data)
    {
        vclose(f);
        Sys_Error("vc_PlayFLI: Not enough memory to play FLI.");
    }
    vread(data, n, f);
    vclose(f);
    
    play_memory_fli(data, &flibuf, 0, ShowPage);
    
    vfree(data);
    
    timer_count=0;
    set_intensity(63);
*/
}

void vc_GotoXY()
{
    int x=ResolveOperand();
    int y=ResolveOperand();

    font.GotoXY(x,y);
}

void vc_PrintString()
{
    string_k	text;
    int		font_slot;
    
    font_slot=ResolveOperand();
    text=ResolveString();
    
    font.PrintString(font_slot, text.c_str());
}

void vc_LoadRaw()
{
    string_k	raw_filename;
    VFILE*	vf;
    int		n;
    char*	ptr;
    
    raw_filename	=ResolveString();
    
    vf=vopen(raw_filename.c_str());
    if (!vf)
    {
        Sys_Error("vc_LoadRaw: could not open file %s", raw_filename.c_str());
    }
    n=filesize(vf);
    ptr=(char *)valloc(n, "LoadRaw:t", OID_VC);
    if (!ptr)
    {
        Sys_Error("vc_LoadRaw: memory exhausted on ptr");
    }
    vread(ptr, n, vf);
    vclose(vf);
    
    vcreturn=(int)ptr;
}

// TODO: rename the layer[] and layers[] arrays less-confusingly. ;P
void vc_SetTile()
{
    int		x, y, lay, value;
    
    x		=ResolveOperand();
    y		=ResolveOperand();
    lay		=ResolveOperand();
    value	=ResolveOperand();
    
    // ensure all arguments are valid
    if (x<0 || y<0)
        return;
    if (lay==6 || lay==7)
    {
        if (x>=layer[0].sizex || y>=layer[0].sizey)
            return;
    }
    else
    {
        if ((lay>=0 && lay<6)
            && (x>=layer[lay].sizex || y>=layer[lay].sizey))
        {
            return;
        }
    }
    
    // determine action
    switch (lay)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        layers[lay][y*layer[lay].sizex+x] = (short)value;
        break;
        
    case 6:
        obstruct[y*layer[0].sizex+x] = (u8)value;
        break;
    case 7:
        zone[y*layer[0].sizex+x] = (u8)value;
        break;
        
    default:
        Sys_Error("vc_SetTile: invalid layer value (");
    }
}

void vc_AllowConsole()
{
}

void vc_ScaleSprite()
{
    int		x, y, source_width, source_length, dest_width, dest_length;
    int		ptr;
    
    x				=ResolveOperand();
    y				=ResolveOperand();
    source_width	=ResolveOperand();
    source_length	=ResolveOperand();
    dest_width		=ResolveOperand();
    dest_length		=ResolveOperand();
    ptr				=ResolveOperand();
    
    if (!lucentmode)
        gfx.ScaleSprite(x,y,source_width,source_length,dest_width,dest_length,(u8*)ptr);
    else
        gfx.ScaleSpriteLucent(x,y,source_width,source_length,dest_width,dest_length,(u8*)ptr,lucentmode);
}

void vc_UpdateControls()
{
    CheckMessages();
}

void vc_Unpress()
{
    input.UnPress(ResolveOperand());
}

void vc_EntityMove()
{
    int		ent;
    string_k	movescript;
    
    ent=ResolveOperand();
    movescript=ResolveString();
    
    if (ent<0 || ent>=entities)
    {
        Log::Write(va("vc_EntityMove: no such entity %d", ent));
        return;
    }

    ents[ent].SetMoveScript(-1337,MoveScript(movescript));
    ents[ent].movecode=Entity::mp_scripted;
}

void vc_HLine()
{
    int		x, y, xe, color;
    
    x		=ResolveOperand();
    y		=ResolveOperand();
    xe		=ResolveOperand();
    color	=ResolveOperand();
    
    gfx.HLine(x,y,xe,color,lucentmode);
}

void vc_VLine()
{
    int		x, y, ye, color;
    
    x		=ResolveOperand();
    y		=ResolveOperand();
    ye		=ResolveOperand();
    color	=ResolveOperand();
    
    gfx.VLine(x,y,ye,color,lucentmode);
}

void vc_Line()
{
    int		x, y, xe, ye, color;
    
    x		=ResolveOperand();
    y		=ResolveOperand();
    xe		=ResolveOperand();
    ye		=ResolveOperand();
    color	=ResolveOperand();
    
    gfx.Line(x, y, xe, ye, color, lucentmode);
}

void vc_Circle()
{
    int		x, y, radius, color;
    
    x		=ResolveOperand();
    y		=ResolveOperand();
    radius	=ResolveOperand();
    color	=ResolveOperand();
    
    gfx.Circle(x, y, radius, color, lucentmode);
}

void vc_CircleFill()
{
    int		x, y, radius, color;
    
    x		=ResolveOperand();
    y		=ResolveOperand();
    radius	=ResolveOperand();
    color	=ResolveOperand();
    
    gfx.CircleFill(x, y, radius, color, lucentmode);
}

void vc_Rect()
{
    int		x, y, xe, ye, color;
    
    x		=ResolveOperand();
    y		=ResolveOperand();
    xe		=ResolveOperand();
    ye		=ResolveOperand();
    color	=ResolveOperand();
    
    
    gfx.Rect(x,y,xe,ye,color,lucentmode);
}

void vc_RectFill()
{
    int		x, y, xe, ye, color;
    
    x		=ResolveOperand();
    y		=ResolveOperand();
    xe		=ResolveOperand();
    ye		=ResolveOperand();
    color	=ResolveOperand();
    
    gfx.RectFill(x,y,xe,ye,color,lucentmode);
}

void vc_strlen()
{
    string_k	text;
    
    text	=ResolveString();
    
    vcreturn=text.length();
}

void vc_strcmp()
{
    string_k	a, b;
    
    a	=ResolveString();
    b	=ResolveString();
    
    if (a<b)
        vcreturn=-1;
    else if (a>b)
        vcreturn=+1;
    else
        vcreturn=0;
}

void vc_CDStop()
{
}

void vc_CDPlay()
{
    ResolveOperand();
}

void vc_FontWidth()
{
    int		n;
    
    n	=ResolveOperand();	// slot
    
    vcreturn=font[n].Width();
}

void vc_FontHeight()
{
    int		n;
    
    n	=ResolveOperand();
    
    vcreturn=font[n].Height();
}

void vc_SetPixel()
{
    int		x, y, color;
    
    x		=ResolveOperand();
    y		=ResolveOperand();
    color	=ResolveOperand();
    
    gfx.SetPixel(x, y, color, lucentmode);
}

void vc_GetPixel()
{
    int		x, y;
    
    x	= ResolveOperand();
    y	= ResolveOperand();
    
    vcreturn=0;
    vcreturn = gfx.GetPixel(x, y);
}

void vc_EntityOnScreen()
{
    int		find, n;
    
    find=ResolveOperand();
    for (n=0; n<cc; n++)
    {
        if (entidx[n]==find)
        {
            vcreturn=1;
            return;
        }
    }
    
    vcreturn=0;
}

void vc_Rand()
{
    int i=ResolveOperand();
    if (i)
        vcreturn=rand()%i;
    else 
        vcreturn=0;
}

void vc_GetTile()
{
    int x, y, lay;
    
    x	=ResolveOperand();
    y	=ResolveOperand();
    lay	=ResolveOperand();
    
    vcreturn=0;
    
    // ensure all arguments are valid
    if (x<0 || y<0)
        return;
    if (lay==6 || lay==7)
    {
        if (x>=layer[0].sizex || y>=layer[0].sizey)
            return;
    }
    else
    {
        if ((lay>=0 && lay<6)
            && (x>=layer[lay].sizex || y>=layer[lay].sizey))
        {
            return;
        }
    }
    
    switch (lay)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        vcreturn=(int)layers[lay][y*layer[lay].sizex+x];
        break;
    case 6:
        vcreturn=(int)obstruct[y*layer[0].sizex+x];
        break;
    case 7:
        vcreturn=(int)zone[y*layer[0].sizex+x];
        break;
        
    default:
        Sys_Error("vc_GetTile: invalid layer value");
    }
}

void vc_SetResolution()
{
    int		xres, yres;
    
    xres	= ResolveOperand();
    yres	= ResolveOperand();
    
    vcreturn =gfx.SetMode(xres, yres,gfx.bpp*8,gfx.IsFullScreen());
    if (!vcreturn) Sys_Error("vc_SetResolution failed");
    gfx.Clear();
    input.ClipMouse(SRect(0,0,xres,yres));
}

void vc_SetRString()
{
    // TODO: some validity checks would be nice
    rstring=ResolveString();
}

void vc_SetClipRect()
{
    int         bogus;
    SRect        clip;
    
    clip.left   = ResolveOperand();
    clip.top    = ResolveOperand();
    clip.right  = ResolveOperand();
    clip.bottom = ResolveOperand();
    
    bogus = 0;
    
    // ensure arguments stay valid
    if (clip.left < 0)              clip.left = 0,             bogus++;
    else if (clip.left >= gfx.scrx) clip.left = gfx.scrx - 1,  bogus++;
    
    if (clip.top < 0)               clip.top = 0,              bogus++;
    else if (clip.top >= gfx.scry)  clip.top = gfx.scry - 1,   bogus++;
    
    if (clip.right<0)               clip.right=0,              bogus++;
    else if (clip.right>=gfx.scrx)  clip.right=gfx.scrx-1,     bogus++;
    
    if (clip.bottom<0)              clip.bottom=0,             bogus++;
    else if (clip.bottom>=gfx.scry) clip.bottom=gfx.scry-1,    bogus++;
    
    if (bogus)
        Log::Write(va("vc_SetClipRect: %d bogus args", bogus));
    gfx.SetClipRect(clip);
}

void vc_SetRenderDest()
{
    u8* scr;
    int x,y;
    
    x=ResolveOperand();
    y=ResolveOperand();
    scr=(u8*)ResolveOperand();
    gfx.SetRenderDest(x,y,scr);
}

void vc_RestoreRenderSettings()
{
    gfx.RestoreRenderSettings();
}

void vc_PartyMove()
{
    extern void vmainloop();

    string_k script=ResolveString();
    int p=playeridx;
    playeridx=-1;

    ents[p].SetMoveScript(-1337,MoveScript(script));
    ents[p].movecode=Entity::mp_scripted;
    cameratracking=2;
    tracker=p;

    timer_count=0;

    // movecode becomes mp_stopped when the script terminates.
    while (ents[p].movecode==Entity::mp_scripted)
        vmainloop();

    cameratracking=1;

    playeridx=p;
}

void vc_sin()
{
    vcreturn=sintbl[ResolveOperand()%360];
}

void vc_cos()
{
    vcreturn=costbl[ResolveOperand()%360];
}

void vc_tan()
{
    vcreturn=tantbl[ResolveOperand()%360];
}

void vc_ReadMouse()
{
    CheckMessages();
}

void vc_SetClipping()
{
    ResolveOperand();   // not a chance, dork.
}

void vc_SetLucent()
{
    lucentmode=ResolveOperand();
}

void vc_WrapBlit()
{
    int		offsetx, offsety, width, length;
    int		ptr;
    
    offsetx	= ResolveOperand();
    offsety	= ResolveOperand();
    width	= ResolveOperand();
    length	= ResolveOperand();
    ptr		= ResolveOperand();
    
    if (!lucentmode)
        gfx.WrapBlit(offsetx,offsety,width,length,(u8*)ptr);
    else
        gfx.WrapBlitLucent(offsetx,offsety,width,length,(u8*)ptr,lucentmode);
}


void vc_TWrapBlit()
{
    int		offsetx, offsety, width, length;
    int		ptr;
    
    offsetx	= ResolveOperand();
    offsety	= ResolveOperand();
    width	= ResolveOperand();
    length	= ResolveOperand();
    ptr		= ResolveOperand();
    
    if (!lucentmode)
        gfx.TWrapBlit(offsetx,offsety,width,length,(u8*)ptr);
    else
        gfx.TWrapBlitLucent(offsetx,offsety,width,length,(u8*)ptr,lucentmode);
}

void vc_SetMousePos()
{
    int		x, y;
    
    x	= ResolveOperand();
    y	= ResolveOperand();
    
    input.MoveMouse(x,y);
}

void vc_HookRetrace()
{
    int		script;
    
    script = 0;
    switch (GrabC())
    {
    case 1:
        script = ResolveOperand();
        break;
    case 2:
        script = USERFUNC_MARKER + GrabD();
        break;
    }
    hookretrace = script;
}

void vc_HookTimer()
{
    int		script;
    
    script = 0;
    switch (GrabC())
    {
    case 1: script = ResolveOperand(); break;
    case 2: script = USERFUNC_MARKER + GrabD(); break;
    }
    hooktimer = script;
}

void vc_HookKey()
{
    int		key, script;
    
    key = ResolveOperand();
    if (key < 0) key = 0;
    if (key > 127) key = 127;
    //	key = scantokey[key];
    
    script = 0;
    switch (GrabC())
    {
    case 1:
        script = ResolveOperand();
        break;
    case 2:
        script = USERFUNC_MARKER + GrabD();
        break;
    }
    bindarray[key] = script;
}

void vc_PlayMusic()
{
    string_k	filename;
    
    filename	= ResolveString();
    
    PlayMusic(filename.c_str());
}

int morph_step(int S, int D, int mix, int light)
{
    return (mix*(S - D) + (100*D))*light/100/64;
}

void vc_PaletteMorph()
{
    int		r,g,b;
    int		percent, intensity;
    
    r = ResolveOperand();	// red
    g = ResolveOperand();	// green
    b = ResolveOperand();	// blue
    
    percent		= 100 - ResolveOperand();
    intensity	= ResolveOperand();
    
    gfx.PaletteMorph(r,g,b,percent,intensity);
}

string_k EnforceNoDirectories(string_k s)
{
    /* 
     * Ff the first char is a backslash, the second char is a colon, or
     * there are two or more consecutive periods anywhere in the string,
     * then bomb out with an error.  You deserve it, you naughty naughty
     * kidide h4x0r.
     */
    
    if (s[0]=='/' || s[0]=='\\')
        Sys_Error(va("EnforceNoDirectories: Invalid file name (%s)",s.c_str()));
    
    if (s[1]==':') // second char a colon?  (eg C:autoexec.bat)
        Sys_Error(va("EnforceNoDirectories: Invalid file name (%s)",s.c_str()));
    int n=0;
    while (n<s.length()-1)
    {
        if (s[n]=='.' && s[n+1]=='.') // two (or more) consective periods?  (eg ..\autoexec.bat)
            Sys_Error(va("EnforceNoDirectories: Invalid file name (%s)",s.c_str()));
        n++;
    }
    
    return s; // We're clean! - tSB
}

namespace
{
    // ----------------------- VC file interface -----------------------

    const int MAXVCFILES=10; // maximum number of files that can be open at once. (through VC)

    struct VC_file
    {
        VFILE* readp; // vfile, for reading
        FILE*  writep;// conventional file, for writing
        int    mode;  // 0 - closed, 1 - read, 2 - write

        VC_file() : readp(0),writep(0),mode(0)
        {}
    };

    VC_file vcfiles[MAXVCFILES];

    int OpenVCFile(const char* fname)
    // Opens a VC file for reading, and returns the index to vcfiles[], or 0 on failure
    // note that vcfiles[0] is a dummy, and isn't actually used anywhere
    {
        for (int i=1; i<MAXVCFILES; i++)
        {
            if (vcfiles[i].mode==0)
            {
                vcfiles[i].readp=vopen(fname);
            
                if (!vcfiles[i].readp) return 0; // file ain't there?
            
                vcfiles[i].mode=1;
                return i;
            }
        }  
        return 0;
    }

    int OpenWriteVCFile(const char* fname)
    // Opens a VC file for writing, and returns the index to vcfiles[], or 0 on failure
    {
        for (int i=1; i<MAXVCFILES; i++)
        {
            if (vcfiles[i].mode==0)
            {
                vcfiles[i].writep=fopen(fname,"wb");
            
                if (!vcfiles[i].writep) return 0; // dunno, sharing violation?
            
                vcfiles[i].mode=2;
                return i;
            }
        }
        return 0;
    }

    void CloseVCFile(int index)
    // closes the specified file, if it's open
    {
        if (vcfiles[index].mode==1)
            vclose(vcfiles[index].readp);
        else if (vcfiles[index].mode==2)
            fclose(vcfiles[index].writep);
        vcfiles[index].mode=0;
    }

    VFILE* GetReadFilePtr(int idx)
    // returns the VFILE, if idx is open for reading, returns an error otherwise
    {
        if (idx<0 || idx>=MAXVCFILES)
            Sys_Error(va("GetReadFilePtr: Invalid file handle %i",idx));
        switch (vcfiles[idx].mode)
        {
        case 0: Sys_Error("GetReadFilePtr: Attempt to read from a closed file.");
        case 2: Sys_Error("GetReadFilePtr: Attempt to read from a file opened for writing.");
        }
        return vcfiles[idx].readp;
    }

    FILE* GetWriteFilePtr(int idx)
    // returns the FILE, if idx is open for writing, returns an error otherwise
    {
        if (idx<0 || idx>=MAXVCFILES)
            Sys_Error(va("GetWriteFilePtr: Invalid file handle %i",idx));
        switch (vcfiles[idx].mode)
        {
        case 0: Sys_Error("GetWriteFilePtr: Attempt to write to a closed file.");
        case 1: Sys_Error("GetWriteFilePtr: Attempt to write to a file opened for reading.");
        }
        return vcfiles[idx].writep;
    }
};

void vc_OpenFile()
{
    string_k	filename = EnforceNoDirectories(ResolveString());
    
    int idx=OpenVCFile(filename.c_str());
    vcreturn = (u32)idx;
    
    Log::Write(va(" --> VC opened file %s, Handle %i", filename.c_str(), idx));
}

void vc_CloseFile()
{
    int idx=ResolveOperand();
    CloseVCFile(idx);
    
    Log::Write(va(" --> VC closed file %i", idx));
}

void vc_QuickRead()
{
    string_k	filename, ret;
    int		offset, seekline;
    
    filename	=ResolveString();
    filename	=EnforceNoDirectories(filename);
    
    // which string are we reading into?
    char code=GrabC();
    if (code==op_STRING)
    {
        offset=GrabW();
    }
    if (code==op_SARRAY)
    {
        offset=GrabW();
        offset+=ResolveOperand();
    }
    
    // which line are we reading from the file?
    seekline = ResolveOperand();
    if (seekline < 1)
        seekline = 1;
    
    // open the file
    VFILE* f=vopen(filename.c_str());
    if (!f)
    {
        Sys_Error("vc_QuickRead: could not open %s", filename.c_str());
    }
    
    // seek to the line of interest
    char temp[256]={0};
    for (int n=0; n<seekline; n++)
    {
        vgets(temp, 255, f);
    }
    // suppress trailing CR/LF
    char *p=temp;
    while (*p)
    {
        if ('\n' == *p || '\r' == *p)
            *p = '\0';
        p++;
    }
    
    // assign to vc string
    if (offset>=0 && offset<stralloc)
    {
        vc_strings[offset]=temp;
    }
    
    vclose(f);
}

void vc_AddFollower()
{
    Log::Write("vc_AddFollower disabled.");
    
    ResolveOperand();	// entity

    /*
    int n=ResolveOperand();
    if (n<0 || n>=entities)
    {
        Sys_Error("vc_AddFollower: Not a valid entity index. (%d)", n);
    }
    
    follower[(int)numfollowers++]=n;
    ResetFollowers();
    */
}

void vc_KillFollower()
{
    ResolveOperand();
}

void vc_KillAllFollowers()
{
}

void vc_ResetFollowers()
{
}

void vc_FlatPoly()
{
    int a,b,c,d,e,f,g;
    
    a=ResolveOperand();	// a
    b=ResolveOperand();	// b
    c=ResolveOperand();	// c
    d=ResolveOperand();	// d
    e=ResolveOperand();	// e
    f=ResolveOperand();	// f
    g=ResolveOperand();	// g
    
    gfx.FlatPoly(a,b,c,d,e,f,g);
}

void vc_TMapPoly()
{
    int a,b,c,d,e,f,g,h,i,j,k,l,m,n,o;
    
    a=ResolveOperand();
    b=ResolveOperand();
    c=ResolveOperand();
    d=ResolveOperand();
    e=ResolveOperand();
    f=ResolveOperand();
    g=ResolveOperand();
    h=ResolveOperand();
    i=ResolveOperand();
    j=ResolveOperand();
    k=ResolveOperand();
    l=ResolveOperand();
    m=ResolveOperand();
    n=ResolveOperand();
    o=ResolveOperand();
    gfx.TMapPoly(a,b,c,d,e,f,g,h,i,j,k,l,m,n,(u8 *) o);
}

void vc_CacheSound()
{
    string_k	filename;
    
    filename	=ResolveString();
    
    vcreturn=CacheSound(filename.c_str());
}

void vc_PlaySound()
{
    int		slot, volume, pan;
    
    slot	=ResolveOperand();
    volume	=ResolveOperand();
    pan		=ResolveOperand();
    
    PlaySFX(slot, volume, pan);
}

void vc_RotScale()
{
    int		x, y, width, length, angle, scale;
    int		ptr;
    
    x		= ResolveOperand();
    y		= ResolveOperand();
    width	= ResolveOperand();
    length	= ResolveOperand();
    angle	= ResolveOperand();
    scale	= ResolveOperand();
    ptr		= ResolveOperand();
    
    if (!lucentmode)
        gfx.RotScale(x,y,width,length,(float)(angle*3.14159/180.0),(float)(scale/1000.0f),(u8*)ptr);
    else
        gfx.RotScaleLucent(x,y,width,length,(float)(angle*3.14159/180.0),(float)(scale/1000.0f),(u8*)ptr,lucentmode);
}

namespace
{
    class OpaqueBlit
    {
    public:
        inline void operator ()(int x,int y,int width,int height,u8* ptr) const
        {
            gfx.CopySprite(x,y,width,height,ptr);
        }
    };

    class TBlit
    {
    public:
        inline void operator ()(int x,int y,int width,int height,u8* ptr) const
        {
            gfx.TCopySprite(x,y,width,height,ptr);
        }
    };

    template <class T>
    void MapLine(int x,int y,int yoffset,int lay,const T& Blit)
    // blah.  poo.
    // The graphics routines are separate from the rest of VERGE, so it's all gotta be here. :P
    // I love templates, by the way. <:D
    {
        if (lay < 0 || lay >= numlayers) return;  // validate arguments
        if (!layertoggle[lay])           return;  // is this layer visible?
        
        // This is the location of the first tile we draw, it'll be at the left hand edge of the screen
        int x_map = (xwin + x)       * layer[lay].pmultx / layer[lay].pdivx;
        int y_map = (ywin + yoffset) * layer[lay].pmulty / layer[lay].pdivy;
        
        yoffset&=15;
        
        if (x_map < 0) x_map = 0;                 // make my life easier; don't allow scrolling past map edges
        if (y_map < 0) y_map = 0;
        
        int x_sub = -(x_map & 15);                // get subtile position while we still have pixel precision
        int y_sub =  (y_map & 15);
        
        x_map >>= 4;                              // determine upper left tile coords of camera 
        y_map >>= 4;
        
        u16* source=layers[lay]+y_map*layer[lay].sizex + x_map; // ew
        // TODO: GetTile function (inline!)
        x=x_sub;
        
        do
        {
            if (*source)
                Blit(x,y,16,1,(u8*)vsp + (16*16* (*source) + (16*y_sub))*gfx.bpp);
            
            source += 1;
            x += 16;
        }
        while (x<gfx.XRes());
    }
}

void vc_MapLine()
// blah.  poo.
// The graphics routines are separate from the rest of VERGE, so it's all gotta be here. :P
{
    // x        = the offset of xwin that we'll use
    // y        = the y coord that we'll draw on
    // y_offset = the y coord that we'll be rendering
    // lay      = the layer number.
    // the line is always the whole screen wide, but at any height and any distortion level.
    int x        = ResolveOperand();
    int y        = ResolveOperand();
    int y_offset = ResolveOperand();
    int lay      = ResolveOperand();

    MapLine(x,y,y_offset,lay,OpaqueBlit());
}

void vc_TMapLine()
{
    // x        = the offset of xwin that we'll use
    // y        = the y coord that we'll draw on
    // y_offset = the y coord that we'll be rendering
    // lay      = the layer number.
    // the line is always the whole screen wide, but at any height and any distortion level.
    int x        = ResolveOperand();
    int y        = ResolveOperand();
    int y_offset = ResolveOperand();
    int lay      = ResolveOperand();

    MapLine(x,y,y_offset,lay,TBlit());
}

void vc_val()
{
    string_k	s;
    
    s	=ResolveString();
    
    vcreturn=s.toint();
}

void vc_TScaleSprite()
{
    int		x, y, source_width, source_length, dest_width, dest_length;
    int		ptr;
    
    x				=ResolveOperand();
    y				=ResolveOperand();
    source_width	=ResolveOperand();
    source_length	=ResolveOperand();
    dest_width		=ResolveOperand();
    dest_length		=ResolveOperand();
    ptr				=ResolveOperand();
    
    if (lucentmode)
        gfx.TScaleSpriteLucent(x, y, source_width, source_length, dest_width, dest_length, (u8 *)ptr,
        lucentmode);
    else
        gfx.TScaleSprite(x, y, source_width, source_length, dest_width, dest_length, (u8 *)ptr);
}

void vc_GrabRegion()
{
    int		x, y, xe, ye, bogus;
    
    x	=ResolveOperand();
    y	=ResolveOperand();
    xe	=ResolveOperand();
    ye	=ResolveOperand();
    
    bogus=0;
    
    // ensure arguments stay valid
    if (x<0)
        x=0,	bogus++;
    else if (x>=gfx.scrx)
        x=gfx.scrx-1,	bogus++;
    if (y<0)
        y=0,	bogus++;
    else if (y>=gfx.scry)
        y=gfx.scry-1,	bogus++;
    
    if (xe<0)
        xe=0,	bogus++;
    else if (xe>=gfx.scrx)
        xe=gfx.scrx-1,bogus++;
    if (ye<0)
        ye=0,	bogus++;
    else if (ye>=gfx.scry)
        ye=gfx.scry-1,bogus++;
    
    if (bogus)
        Log::Write(va("vc_GrabRegion: %d bogus args", bogus));
    
    // swap?
    if (xe<x) {
        int t=x;
        x=xe;
        xe=t;
    }
    if (ye<y) {
        int t=ye;
        y=ye;
        ye=t;
    }
    
    xe = xe - x + 1;
    ye = ye - y + 1;
    
    if (gfx.bpp>1)
    {
        unsigned short* source;
        unsigned short* ptr;
        int n;
        
        source = ((unsigned short *) gfx.screen) + (y*gfx.scrx) + x;
        ptr = (unsigned short *) ResolveOperand();
        
        while (ye)
        {
            for (n = 0; n < xe; n += 1)
                ptr[n] = source[n];
            
            ptr += xe;
            source += gfx.scrx;
            ye -= 1;
        }
    }
    else
    {
        unsigned char* source;
        unsigned char* ptr;
        
        source = gfx.screen + (y*gfx.scrx) + x;
        ptr = (unsigned char *) ResolveOperand();
        
        while (ye)
        {
            memcpy(ptr, source, xe);
            
            ptr += xe;
            source += gfx.scrx;
            ye -= 1;
        }
    }
}

void vc_Log()
{
    string_k	message;
    
    message	=ResolveString();
    
    Log::Write(message.c_str());
}

void vc_fseekline()
{
    int		line;
    VFILE*	vf;
    
    line	=ResolveOperand()-2;						// Unfixed, so that it behaves like v2k+j
    vf		=GetReadFilePtr(ResolveOperand());
    
    vseek(vf, 0, SEEK_SET);
    
    // 1 will yield first line
    char temp[256+1];
    //	do
    while (line-->0)
    {
        vgets(temp, 256, vf);
    }// while (--line > 0);
}

void vc_fseekpos()
{
    int		pos;
    VFILE*	vf;
    
    pos	=ResolveOperand();
    vf	=GetReadFilePtr(ResolveOperand());
    
    vseek(vf, pos, 0);
}

void vc_fread()
{
    char*	buffer;
    int		len;
    VFILE*	vf;
    
    buffer	=(char *)ResolveOperand();
    len		=ResolveOperand();
    vf		=GetReadFilePtr(ResolveOperand());
    
    vread(buffer, len, vf);
}

void vc_fgetbyte()
{
    VFILE*	vf	=0;
    u8	b	=0;
    
    vf=GetReadFilePtr(ResolveOperand());
    vread(&b, 1, vf);
    
    vcreturn=b;
}

void vc_fgetword()
{
    VFILE*	vf	=0;
    u16	w	=0;
    
    vf=GetReadFilePtr(ResolveOperand());
    vread(&w, 2, vf);
    
    vcreturn=w;
}

void vc_fgetquad()
{
    VFILE*	vf	=0;
    u32		q	=0;
    
    vf=GetReadFilePtr(ResolveOperand());
    vread(&q, 4, vf);
    
    vcreturn=q;
}

void vc_fgetline()
{
    char	temp[256 +1];
    char*	p;
    int		code, offset;
    VFILE*	vf;
    
    // which global vc string do we read into?
    code=GrabC();
    offset=GrabW();
    if (op_SARRAY == code)
    {
        offset+=ResolveOperand();
    }
    
    // file pointer; blegh
    vf=GetReadFilePtr(ResolveOperand());
    
    // read line into temp buffer
    vgets(temp, 256, vf);
    temp[256]='\0';
    
    // suppress trailing CR/LF
    p=temp;
    while (*p)
    {
        if ('\n' == *p || '\r' == *p)
            *p = '\0';
        p++;
    }
    
    // assign to vc string
    if (offset>=0 && offset<stralloc)
    {
        vc_strings[offset]=temp;
    }
}

void vc_fgettoken()
{
    char	temp[256];
    int		code, offset;
    VFILE*	vf;
    
    // which global vc string do we read into?
    code=GrabC();
    offset=GrabW();
    if (code == op_SARRAY)
    {
        offset+=ResolveOperand();
    }
    
    // file pointer; blegh
    vf=GetReadFilePtr(ResolveOperand());
    
    // read into temp buffer
    vscanf(vf, "%s", temp);
    
    // assign to vc string
    if (offset>=0 && offset<stralloc)
    {
        vc_strings[offset]=temp;
    }
}

void vc_fwritestring()
{
    FILE*	f;
    string_k	temp;
    
    temp	=ResolveString();
    f=GetWriteFilePtr(ResolveOperand());
    
    fprintf(f, "%s\n", temp.c_str());
}

void vc_fwrite()
{
    char*	buffer;
    int		length;
    FILE*	f;
    
    buffer	=(char *)ResolveOperand();
    length	=ResolveOperand();

    f=GetWriteFilePtr(ResolveOperand());
    
    fwrite(buffer, 1, length, f);
}

void vc_frename()
{
    string_k	a, b;
    
    a=ResolveString();
    b=ResolveString();
    a=EnforceNoDirectories(a);
    b=EnforceNoDirectories(b);
    
    rename(a.c_str(), b.c_str());
    
    Log::Write(va(" --> VC renamed %s to %s.", a.c_str(), b.c_str()));
}

void vc_fdelete()
{
    string_k	filename;
    
    filename=ResolveString();
    filename=EnforceNoDirectories(filename);
    
    remove(filename.c_str());
    
    Log::Write(va(" --> VC deleted %s.", filename.c_str()));
}

void vc_fwopen()
{
    string_k	filename;
    
    filename	=ResolveString();
    filename	=EnforceNoDirectories(filename);
    
    vcreturn=OpenWriteVCFile(filename.c_str());
    
    Log::Write(va(" --> VC opened %s for writing, handle %i.", filename.c_str(), vcreturn));
}

void vc_fwclose()
{
    int idx=ResolveOperand();
    CloseVCFile(idx);
    
    Log::Write(va(" --> VC close file opened for writing, handle %i.", idx));
}

void vc_memcpy()
{
    char	*source, *dest;
    int		length;
    
    dest	=(char *)ResolveOperand();
    source	=(char *)ResolveOperand();
    length	=ResolveOperand();
    
    memcpy(dest, source, length);
}

void vc_memset()
{
    char*	dest;
    int		color, length;
    
    dest	=(char *)ResolveOperand();
    color	=ResolveOperand();
    length	=ResolveOperand();
    
    memset(dest, color, length);
}

// <aen, may 5>
// + modified to use new silhouette vdriver routines
// + added checks for ClipOn
void vc_Silhouette()
{
    int width,height,src,dest,colour;
    width  = ResolveOperand();
    height = ResolveOperand();
    src    = ResolveOperand();
    dest   = ResolveOperand();
    colour = ResolveOperand();
    
    gfx.Silhouette(width,height,(u8*)src,(u8*)dest,colour);
}

void vc_InitMosaicTable()
{
}

void vc_Mosaic()
{
    Log::Write("vc_Mosaic disabled.");
    
    ResolveOperand();	// a
    ResolveOperand();	// b
    ResolveOperand();	// c
    ResolveOperand();	// d
    ResolveOperand();	// e
    ResolveOperand();	// f
    ResolveOperand();	// g

    /*
    int a,b,c,d,e,f,g;
    
    a=ResolveOperand();
    b=ResolveOperand();
    c=ResolveOperand();
    d=ResolveOperand();
    e=ResolveOperand();
    f=ResolveOperand();
    g=ResolveOperand();
    Mosaic(a,b,(u8 *) c,d,e,f,g);
    */
}

void vc_WriteVars()
{
    FILE*	f;
    
    f=GetWriteFilePtr(ResolveOperand());
    
    fwrite(globalint, 1, 4*maxint, f);
    
    for (int n=0; n<stralloc; n++)
    {
        int z=vc_strings[n].length();
        
        fwrite(&z, 1, 4, f);      
        fwrite(vc_strings[n].c_str(), 1, z, f);
    }
}

void vc_ReadVars()
{
    VFILE*	f;
    
    f=GetReadFilePtr(ResolveOperand());
    
    vread(globalint, 4*maxint, f);
    
    for (int n=0; n<stralloc; n++)
    {
        int z;
        vread(&z, 4, f);
        
        char* temp=new char[z+1];
        if (!temp)
            Sys_Error("vc_Readars: memory exhausted on %d bytes.", z);
        vread(temp, z, f);
        temp[z]='\0';
        vc_strings[n]=temp;
        
        delete[] temp;
        temp=0;
    }
}

void vc_CallEvent()
{
    ExecuteEvent(ResolveOperand());
}

void vc_Asc()
{
    vcreturn=ResolveString()[0];
}

void vc_CallScript()
{
    ExecuteUserFunc(ResolveOperand());
}

void vc_NumForScript()
{
    vcreturn=GrabD();
}

void vc_FileSize()
{
    string_k	filename;
    VFILE*	vf;
    
    filename	=ResolveString();
    
    vf=vopen(filename.c_str());
    vcreturn=filesize(vf);
    vclose(vf);
}

void vc_ftell()
{
    VFILE*	vf;
    
    vf=GetReadFilePtr(ResolveOperand());
    
    vcreturn=vtell(vf);
}

void vc_CheckCorrupt()
{
    //	Log::Write("vc_CheckCorrupt disabled.");
    
    //*
    Log::Write("checking for corruption...");
    CheckCorruption();
    //*/
}

void vc_ChangeCHR()
{
    int		who;
    string_k	chrname;
    
    who=ResolveOperand();
    chrname=ResolveString();
    
    ChangeCHR(who, chrname.c_str());
}

void vc_RGB()
{
    int r, g, b;
    
    r = ResolveOperand();
    g = ResolveOperand();
    b = ResolveOperand();
    
    vcreturn
        = gfx.PackPixel(r,g,b);
}

// Gah!  replace these with vc_unpackpixel! --tSB
void vc_GetR()
{
    int color;
    int r,g,b;
    
    color = ResolveOperand();
    gfx.UnPackPixel(color,r,g,b);
    
    vcreturn = r;
}

void vc_GetG()
{
    int color;
    int r,g,b;
    
    color = ResolveOperand();
    gfx.UnPackPixel(color,r,g,b);
    
    vcreturn = g;
}

void vc_GetB()
{
    int color;
    int r,g,b;
    
    color = ResolveOperand();
    gfx.UnPackPixel(color,r,g,b);
    
    vcreturn = b;
}

void vc_Mask()
{
    int		source, mask, width, length, dest;
    
    source = ResolveOperand();
    mask = ResolveOperand();
    width = ResolveOperand();
    length = ResolveOperand();
    dest = ResolveOperand();
    
    gfx.Mask((u8*)source,(u8*)dest,width,length,(u8*)mask);
}

void vc_ChangeAll()
{
    int		source, width, length, source_color, dest_color;
    
    source = ResolveOperand();
    width = ResolveOperand();
    length = ResolveOperand();
    source_color = ResolveOperand();
    dest_color = ResolveOperand();
    
    gfx.ChangeAll(width,length,(u8*)source,source_color,dest_color);
}

void vc_sqrt()
{
    vcreturn=(int)sqrt(ResolveOperand());
}

//- tSB
void vc_fwritebyte()
{
    char b;
    FILE* f;
    
    b=(char)ResolveOperand();

    f=GetWriteFilePtr(ResolveOperand());
    fwrite(&b,1,1,f);
}

void vc_fwriteword()
{
    u16 b;
    FILE* f;
    
    b=(u16)ResolveOperand();
    
    f=GetWriteFilePtr(ResolveOperand());
    fwrite(&b,1,2,f);
}

void vc_fwritequad()
{
    int b;
    FILE* f;
    
    b=ResolveOperand();
    
    f=GetWriteFilePtr(ResolveOperand());
    fwrite(&b,1,4,f);
}

void vc_CalcLucent()
{
    gfx.CalcLucentLUT(ResolveOperand());
}

void vc_ImageSize()
{
    int x1,x2,y1,y2;
    
    x1=ResolveOperand();
    y1=ResolveOperand();
    x2=ResolveOperand();
    y2=ResolveOperand();
    
    if (x1>x2)
    {
        int i=x1;
        x1=x2;
        x2=i;
    }
    if (y1>y2)
    {
        int i=y1;
        y1=y2;
        y2=i;
    }
    
    x2-=x1;
    y2-=y1;
    vcreturn=x2*y2*gfx.bpp;
}
//- end

VCFunc vcfunctions[]=
{
    0,
    vc_Exit_,               vc_Message,
    vc_Malloc,              vc_Free,
    vc_pow,                 vc_LoadImage,
    vc_CopySprite,          vc_TCopySprite,
    Render,                 vc_ShowPage,
    vc_EntitySpawn,         vc_SetPlayer,
    vc_Map,                 vc_LoadFont,
    vc_PlayFLI,             vc_GotoXY,
    vc_PrintString,         vc_LoadRaw,
    vc_SetTile,             vc_AllowConsole,
    vc_ScaleSprite,         ProcessEntities,
    vc_UpdateControls,      vc_Unpress,
    vc_EntityMove,          vc_HLine,
    vc_VLine,               vc_Line,
    vc_Circle,              vc_CircleFill,
    vc_Rect,                vc_RectFill,
    vc_strlen,              vc_strcmp,
    vc_CDStop,              vc_CDPlay,
    vc_FontWidth,           vc_FontHeight,
    vc_SetPixel,            vc_GetPixel,
    vc_EntityOnScreen,      vc_Rand,
    vc_GetTile,             vc_HookRetrace,
    vc_HookTimer,           vc_SetResolution,
    vc_SetRString,          vc_SetClipRect,
    vc_SetRenderDest,       vc_RestoreRenderSettings,
    vc_PartyMove,           vc_sin,
    vc_cos,                 vc_tan,
    vc_ReadMouse,           vc_SetClipping,
    vc_SetLucent,           vc_WrapBlit,
    vc_TWrapBlit,           vc_SetMousePos,
    vc_HookKey,             vc_PlayMusic,
    StopMusic,              vc_PaletteMorph,
    vc_OpenFile,            vc_CloseFile,
    vc_QuickRead,           vc_AddFollower,
    vc_KillFollower,        vc_KillAllFollowers,
    vc_ResetFollowers,      vc_FlatPoly,
    vc_TMapPoly,            vc_CacheSound,
    FreeAllSounds,          vc_PlaySound,
    vc_RotScale,            vc_MapLine,
    vc_TMapLine,            vc_val,
    vc_TScaleSprite,        vc_GrabRegion,
    vc_Log,                 vc_fseekline,
    vc_fseekpos,            vc_fread,
    vc_fgetbyte,            vc_fgetword,
    vc_fgetquad,            vc_fgetline,
    vc_fgettoken,           vc_fwritestring,
    vc_fwrite,              vc_frename,
    vc_fdelete,             vc_fwopen,
    vc_fwclose,             vc_memcpy,
    vc_memset,              vc_Silhouette,
    vc_InitMosaicTable,     vc_Mosaic,
    vc_WriteVars,           vc_ReadVars,
    vc_CallEvent,           vc_Asc,
    vc_CallScript,          vc_NumForScript,
    vc_FileSize,            vc_ftell,
    vc_ChangeCHR,           vc_RGB,
    vc_GetR,                vc_GetG,
    vc_GetB,                vc_Mask,
    vc_ChangeAll,           vc_sqrt,
    vc_fwritebyte,          vc_fwriteword,
    vc_fwritequad,          vc_CalcLucent,
    vc_ImageSize
};
