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
#include "Map/MapFile.h"
#include "vsp.h"
#include "sincos.h"
#include "verge.h"
#include "log.h"
#include <math.h>
#include <vector>

using Map::MapFile;

extern int ResolveOperand();    // vc.cpp
extern std::string ResolveString();// ditto
extern int vcreturn;

extern u8 GrabC();
extern u32 GrabD();
extern u16 GrabW();

extern std::vector<std::string> vc_strings;
extern std::vector<int> globalint;
extern int maxint;

int lucentmode = 0;

void vmainloop();

namespace VC
{
    void UnPress()
    {
        int             n;
        n = ResolveOperand();

        input.UnPress(n);
    }

    void Exit_()
    {
        std::string     message;

        message = ResolveString();

        Sys_Error(message.c_str());
    }

    void Message()
    {
        std::string     message;
        int             n;

        message = ResolveString();
        n = ResolveOperand();

        Message_Send(message, n);
    }

    void Malloc()
    {
        int             n;

        n = ResolveOperand();

        vcreturn=(int)valloc(n, "vcreturn", OID_VC);

        ::Log::Write(va("VC allocating %u bytes, ptr at 0x%08X.", n, vcreturn));

        if (!vcreturn)
            Message_Send("Warning: VC failed malloc", 750);
    }

    void Free()
    {
        int ptr = ResolveOperand();

        if (ptr)
        {
            vfree((void *)ptr);

            ::Log::Write(va("VC freeing allocated heap at 0x%08X.", ptr));
        }
    }

    void pow()
    {
        int             a, b;

        a = ResolveOperand();
        b = ResolveOperand();

        vcreturn=(int)::pow(a, b);
    }

    void LoadImage()
    {
        std::string     filename = ResolveString();

        vcreturn=(int)Image_LoadBuf(filename.c_str());
        if (!vcreturn)
        {
            Sys_Error("loadimage: %s: unable to open", filename.c_str());
        }
    }

    void CopySprite()
    {
        int             x, y, width, length;
        int             ptr;

        x               =ResolveOperand();
        y               =ResolveOperand();
        width   =ResolveOperand();
        length  =ResolveOperand();
        ptr             =ResolveOperand();

        if (lucentmode)
            gfx.CopySpriteLucent(x, y, width, length, (u8 *)ptr, lucentmode);
        else
            gfx.CopySprite(x, y, width, length, (u8 *)ptr);
    }

    void TCopySprite()
    {
        int             x, y, width, length;
        int             ptr;

        x               =ResolveOperand();
        y               =ResolveOperand();
        width   =ResolveOperand();
        length  =ResolveOperand();
        ptr             =ResolveOperand();

        if (lucentmode)
            gfx.TCopySpriteLucent(x, y, width, length, (u8 *)ptr, lucentmode);
        else
            gfx.TCopySprite(x, y, width, length, (u8 *)ptr);
    }

    void ShowPage()
    {
        gfx.ShowPage();
    }

    void EntitySpawn()
    {
        int             tilex, tiley;
        std::string     chr_filename;

        tilex = ResolveOperand();
        tiley = ResolveOperand();
        chr_filename = ResolveString();

        vcreturn = AllocateEntity(tilex, tiley, chr_filename.c_str());
    }

    void SetPlayer()
    {
        int n = ResolveOperand();
        if (n<0 || n>=entities)
        {
            Sys_Error("SetPlayer: entity index out of range (attempted %d)", n);
        }

        playeridx = n;
        cameratracking = 1;

        ents[n].Stop();
    }

    void Map()
    {
        hookretrace = 0;
        hooktimer = 0;
        vckill = 1;

        startmap = ResolveString();
    }

    void LoadFont()
    {
        std::string     filename = ResolveString();

        vcreturn = font.Load(filename.c_str());
    }

    void PlayFLI()
    {
        ::Log::Write("PlayFLI disabled.");

        ResolveString();
    }

    void GotoXY()
    {
        int x = ResolveOperand();
        int y = ResolveOperand();

        font.GotoXY(x, y);
    }

    void PrintString()
    {
        std::string     text;
        int             font_slot;

        font_slot = ResolveOperand();
        text = ResolveString();

        font.PrintString(font_slot, text.c_str());
    }

    void LoadRaw()
    {
        std::string     raw_filename;
        VFILE*  vf;
        int             n;
        char*   ptr;

        raw_filename    =ResolveString();

        vf = vopen(raw_filename.c_str());
        if (!vf)
        {
            Sys_Error("LoadRaw: could not open file %s", raw_filename.c_str());
        }
        n = filesize(vf);
        ptr=(char *)valloc(n, "LoadRaw:t", OID_VC);
        if (!ptr)
        {
            Sys_Error("LoadRaw: memory exhausted on ptr");
        }
        vread(ptr, n, vf);
        vclose(vf);

        vcreturn=(int)ptr;
    }

    // TODO: rename the layer[] and layers[] arrays less-confusingly. ;P
    void SetTile()
    {
        int             x, y, lay, value;

        x               =ResolveOperand();
        y               =ResolveOperand();
        lay             =ResolveOperand();
        value   =ResolveOperand();

        // ensure all arguments are valid
        if (x<0 || y<0)
            return;
        if (lay==6 || lay==7)
        {
            if (x>=map->Width() || y>=map->Height())
                return;
        }
        else
        {
            if ((lay>=0 && lay<6)
                && (x>=map->GetLayer(lay).Width() || y>=map->GetLayer(lay).Height()))
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
            //layers[lay][y*map->GetLayer(lay).Width()+x] = (short)value;
            map->GetLayer(lay).Set(x, y, value);
            break;

        case 6:
            //obstruct[y*map->Width()+x] = (u8)value;
            map->Obs().Set(x, y, value!=0);
            break;
        case 7:
            //zone[y*map->Width()+x] = (u8)value;
            map->Zone().Set(x, y, value);
            break;

        default:
            Sys_Error("SetTile: invalid layer value (");
        }
    }

    void AllowConsole()
    {
        bool enable = ResolveOperand() != 0;

        if (enable)
            console.Enable();
        else
            console.Disable();
    }

    void ScaleSprite()
    {
        int             x, y, source_width, source_length, dest_width, dest_length;
        int             ptr;

        x               =ResolveOperand();
        y               =ResolveOperand();
        source_width    =ResolveOperand();
        source_length   =ResolveOperand();
        dest_width      =ResolveOperand();
        dest_length     =ResolveOperand();
        ptr             =ResolveOperand();

        if (!lucentmode)
            gfx.ScaleSprite(x, y, source_width, source_length, dest_width, dest_length, (u8*)ptr);
        else
            gfx.ScaleSpriteLucent(x, y, source_width, source_length, dest_width, dest_length, (u8*)ptr, lucentmode);
    }

    void UpdateControls()
    {
        CheckMessages();
        input.Update();
    }

    void Unpress()
    {
        input.UnPress(ResolveOperand());
    }

    void EntityMove()
    {
        int             ent;
        std::string     movescript;

        ent = ResolveOperand();
        movescript = ResolveString();

        if (ent<0 || ent>=entities)
        {
            ::Log::Write(va("EntityMove: no such entity %d", ent));
            return;
        }

        ents[ent].SetMoveScript(MoveScript(movescript));
        ents[ent].movecode = Entity::mp_scripted;
    }

    void HLine()
    {
        int             x, y, xe, color;

        x               =ResolveOperand();
        y               =ResolveOperand();
        xe              =ResolveOperand();
        color   =ResolveOperand();

        gfx.HLine(x, y, xe, color, lucentmode);
    }

    void VLine()
    {
        int             x, y, ye, color;

        x               =ResolveOperand();
        y               =ResolveOperand();
        ye              =ResolveOperand();
        color   =ResolveOperand();

        gfx.VLine(x, y, ye, color, lucentmode);
    }

    void Line()
    {
        int             x, y, xe, ye, color;

        x               =ResolveOperand();
        y               =ResolveOperand();
        xe              =ResolveOperand();
        ye              =ResolveOperand();
        color   =ResolveOperand();

        gfx.Line(x, y, xe, ye, color, lucentmode);
    }

    void Circle()
    {
        int             x, y, radius, color;

        x               =ResolveOperand();
        y               =ResolveOperand();
        radius  =ResolveOperand();
        color   =ResolveOperand();

        gfx.Circle(x, y, radius, color, lucentmode);
    }

    void CircleFill()
    {
        int             x, y, radius, color;

        x               =ResolveOperand();
        y               =ResolveOperand();
        radius  =ResolveOperand();
        color   =ResolveOperand();

        gfx.CircleFill(x, y, radius, color, lucentmode);
    }

    void Rect()
    {
        int             x, y, xe, ye, color;

        x               =ResolveOperand();
        y               =ResolveOperand();
        xe              =ResolveOperand();
        ye              =ResolveOperand();
        color   =ResolveOperand();


        gfx.Rect(x, y, xe, ye, color, lucentmode);
    }

    void RectFill()
    {
        int             x, y, xe, ye, color;

        x               =ResolveOperand();
        y               =ResolveOperand();
        xe              =ResolveOperand();
        ye              =ResolveOperand();
        color   =ResolveOperand();

        gfx.RectFill(x, y, xe, ye, color, lucentmode);
    }

    void strlen()
    {
        std::string     text;

        text    =ResolveString();

        vcreturn = text.length();
    }

    void strcmp()
    {
        std::string     a, b;

        a       =ResolveString();
        b       =ResolveString();

        if (a<b)
            vcreturn=-1;
        else if (a>b)
            vcreturn=+1;
        else
            vcreturn = 0;
    }

    void CDStop()
    {
    }

    void CDPlay()
    {
        ResolveOperand();
    }

    void FontWidth()
    {
        int             n;

        n       =ResolveOperand();      // slot

        vcreturn = font[n].Width();
    }

    void FontHeight()
    {
        int             n;

        n       =ResolveOperand();

        vcreturn = font[n].Height();
    }

    void SetPixel()
    {
        int             x, y, color;

        x               =ResolveOperand();
        y               =ResolveOperand();
        color   =ResolveOperand();

        gfx.SetPixel(x, y, color, lucentmode);
    }

    void GetPixel()
    {
        int             x, y;

        x       = ResolveOperand();
        y       = ResolveOperand();

        vcreturn = 0;
        vcreturn = gfx.GetPixel(x, y);
    }

    void EntityOnScreen()
    {
        int             find, n;

        find = ResolveOperand();
        for (n = 0; n<numentsonscreen; n++)
        {
            if (entidx[n]==find)
            {
                vcreturn = 1;
                return;
            }
        }

        vcreturn = 0;
    }

    void Rand()
    {
        int i = ResolveOperand();
        if (i)
            vcreturn = rand()%i;
        else
            vcreturn = 0;
    }

    void GetTile()
    {
        int x, y, lay;

        x       =ResolveOperand();
        y       =ResolveOperand();
        lay     =ResolveOperand();

        vcreturn = 0;

        // ensure all arguments are valid
        /*if (x<0 || y<0)
            return;

        if (lay==6 || lay==7)
        {
            if (x>=map->Width() || y>=map->Height())
                return;
        }
        else
        {
            if ((lay>=0 && lay<6)
                && (x>=map->GetLayer(lay).Width() || y>=map->GetLayer(lay).Height()))
            {
                return;
            }
        }*/

        switch (lay)
        {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            vcreturn = map->GetLayer(lay).Get(x, y);
            break;
        case 6:
            vcreturn = map->Obs().Get(x, y)?1:0;//(int)obstruct[y*map->Width()+x];
            break;
        case 7:
            vcreturn = map->Zone().Get(x, y);//(int)zone[y*map->Width()+x];
            break;

        default:
            Sys_Error("GetTile: invalid layer value");
        }
    }

    void SetResolution()
    {
        int             xres, yres;

        xres    = ResolveOperand();
        yres    = ResolveOperand();

        vcreturn =gfx.SetMode(xres, yres, gfx.bpp*8, gfx.IsFullScreen());
        if (!vcreturn) Sys_Error("SetResolution failed");
        gfx.Clear();
        input.ClipMouse(SRect(0, 0, xres, yres));
    }

    void SetRString()
    {
        // TODO: some validity checks would be nice
        map->rstring = ResolveString();
    }

    void SetClipRect()
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

        if (clip.right<0)               clip.right = 0,              bogus++;
        else if (clip.right>=gfx.scrx)  clip.right = gfx.scrx-1,     bogus++;

        if (clip.bottom<0)              clip.bottom = 0,             bogus++;
        else if (clip.bottom>=gfx.scry) clip.bottom = gfx.scry-1,    bogus++;

        if (bogus)
            ::Log::Write(va("SetClipRect: %d bogus args", bogus));
        gfx.SetClipRect(clip);
    }

    void SetRenderDest()
    {
        u8* scr;
        int x, y;

        x = ResolveOperand();
        y = ResolveOperand();
        scr=(u8*)ResolveOperand();
        gfx.SetRenderDest(x, y, scr);
    }

    void RestoreRenderSettings()
    {
        gfx.RestoreRenderSettings();
    }

    void PartyMove()
    {
        std::string script = ResolveString();
        int p = playeridx;
        playeridx=-1;

        ents[p].SetMoveScript(MoveScript(script));
        ents[p].movecode = Entity::mp_scripted;
        cameratracking = 2;
        tracker = p;

        timer_count = 0;

        // movecode becomes mp_stopped when the script terminates.
        while (ents[p].movecode==Entity::mp_scripted)
            vmainloop();

        cameratracking = 1;

        playeridx = p;
    }

    void sin()
    {
        vcreturn = sintbl[ResolveOperand()%360];
    }

    void cos()
    {
        vcreturn = costbl[ResolveOperand()%360];
    }

    void tan()
    {
        vcreturn = tantbl[ResolveOperand()%360];
    }

    void ReadMouse()
    {
        CheckMessages();
    }

    void SetClipping()
    {
        ResolveOperand();   // not a chance, dork.
    }

    void SetLucent()
    {
        lucentmode = ResolveOperand();
    }

    void WrapBlit()
    {
        int             offsetx, offsety, width, length;
        int             ptr;

        offsetx = ResolveOperand();
        offsety = ResolveOperand();
        width   = ResolveOperand();
        length  = ResolveOperand();
        ptr             = ResolveOperand();

        if (!lucentmode)
            gfx.WrapBlit(offsetx, offsety, width, length, (u8*)ptr);
        else
            gfx.WrapBlitLucent(offsetx, offsety, width, length, (u8*)ptr, lucentmode);
    }


    void TWrapBlit()
    {
        int             offsetx, offsety, width, length;
        int             ptr;

        offsetx = ResolveOperand();
        offsety = ResolveOperand();
        width   = ResolveOperand();
        length  = ResolveOperand();
        ptr             = ResolveOperand();

        if (!lucentmode)
            gfx.TWrapBlit(offsetx, offsety, width, length, (u8*)ptr);
        else
            gfx.TWrapBlitLucent(offsetx, offsety, width, length, (u8*)ptr, lucentmode);
    }

    void SetMousePos()
    {
        int             x, y;

        x       = ResolveOperand();
        y       = ResolveOperand();

        input.MoveMouse(x, y);
    }

    void HookRetrace()
    {
        int             script;

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

    void HookTimer()
    {
        int             script;

        script = 0;
        switch (GrabC())
        {
        case 1: script = ResolveOperand(); break;
        case 2: script = USERFUNC_MARKER + GrabD(); break;
        }
        hooktimer = script;
    }

    void HookKey()
    {
        int             key, script;

        key = ResolveOperand();
        if (key < 0) key = 0;
        if (key > 127) key = 127;
        //      key = scantokey[key];

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

    void PlayMusic()
    {
        std::string     filename;

        filename        = ResolveString();

        ::PlayMusic(filename.c_str());
    }

    int morph_step(int S, int D, int mix, int light)
    {
        return (mix*(S - D) + (100*D))*light/100/64;
    }

    void PaletteMorph()
    {
        int             r, g, b;
        int             percent, intensity;

        r = ResolveOperand();   // red
        g = ResolveOperand();   // green
        b = ResolveOperand();   // blue

        percent         = 100 - ResolveOperand();
        intensity       = ResolveOperand();

        gfx.PaletteMorph(r, g, b, percent, intensity);
    }

    std::string EnforceNoDirectories(const std::string& s)
    {
        /*
        * If the first char is a backslash, the second char is a colon, or
        * there are two or more consecutive periods anywhere in the string,
        * then bomb out with an error.  You deserve it, you naughty naughty
        * kidide h4x0r.
        */

        if (s[0]=='/' || s[0]=='\\')
            Sys_Error(va("EnforceNoDirectories: Invalid file name (%s)", s.c_str()));

        if (s[1]==':') // second char a colon?  (eg C:autoexec.bat)
            Sys_Error(va("EnforceNoDirectories: Invalid file name (%s)", s.c_str()));

        int p = s.find("..");
        if (p != -1)
            Sys_Error(va("EnforceNoDirectories: Invalid file name (%s)", s.c_str()));

        return s; // We're clean! - tSB
    }

    namespace
    {
        // ----------------------- VC file interface -----------------------

        const int MAXVCFILES = 10; // maximum number of files that can be open at once. (through VC)

        struct file
        {
            VFILE* readp; // vfile, for reading
            FILE*  writep;// conventional file, for writing
            int    mode;  // 0 - closed, 1 - read, 2 - write

            file() : readp(0), writep(0), mode(0)
            {}
        };

        file vcfiles[MAXVCFILES];

        int OpenVCFile(const char* fname)
        // Opens a VC file for reading, and returns the index to vcfiles[], or 0 on failure
        // note that vcfiles[0] is a dummy, and isn't actually used anywhere
        {
            for (int i = 1; i<MAXVCFILES; i++)
            {
                if (vcfiles[i].mode==0)
                {
                    vcfiles[i].readp = vopen(fname);

                    if (!vcfiles[i].readp) return 0; // file ain't there?

                    vcfiles[i].mode = 1;
                    return i;
                }
            }
            return 0;
        }

        int OpenWriteVCFile(const char* fname)
        // Opens a VC file for writing, and returns the index to vcfiles[], or 0 on failure
        {
            for (int i = 1; i<MAXVCFILES; i++)
            {
                if (vcfiles[i].mode==0)
                {
                    vcfiles[i].writep = fopen(fname, "wb");

                    if (!vcfiles[i].writep) return 0; // dunno, sharing violation?

                    vcfiles[i].mode = 2;
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
            vcfiles[index].mode = 0;
        }

        VFILE* GetReadFilePtr(int idx)
        // returns the VFILE, if idx is open for reading, returns an error otherwise
        {
            if (idx<0 || idx>=MAXVCFILES)
                Sys_Error(va("GetReadFilePtr: Invalid file handle %i", idx));
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
                Sys_Error(va("GetWriteFilePtr: Invalid file handle %i", idx));
            switch (vcfiles[idx].mode)
            {
            case 0: Sys_Error("GetWriteFilePtr: Attempt to write to a closed file.");
            case 1: Sys_Error("GetWriteFilePtr: Attempt to write to a file opened for reading.");
            }
            return vcfiles[idx].writep;
        }
    };

    void OpenFile()
    {
        std::string     filename = EnforceNoDirectories(ResolveString());

        int idx = OpenVCFile(filename.c_str());
        vcreturn = (u32)idx;

        ::Log::Write(va(" --> VC opened file %s, Handle %i", filename.c_str(), idx));
    }

    void CloseFile()
    {
        int idx = ResolveOperand();
        CloseVCFile(idx);

        ::Log::Write(va(" --> VC closed file %i", idx));
    }

    void QuickRead()
    {
        std::string     filename, ret;
        int             offset, seekline;

        filename        =ResolveString();
        filename        =EnforceNoDirectories(filename);

        // which string are we reading into?
        char code = GrabC();
        if (code==op_STRING)
        {
            offset = GrabW();
        }
        if (code==op_SARRAY)
        {
            offset = GrabW();
            offset+=ResolveOperand();
        }

        // which line are we reading from the file?
        seekline = ResolveOperand();
        if (seekline < 1)
            seekline = 1;

        // open the file
        VFILE* f = vopen(filename.c_str());
        if (!f)
        {
            Sys_Error("QuickRead: could not open %s", filename.c_str());
        }

        // seek to the line of interest
        char temp[256]={0};
        for (int n = 0; n<seekline; n++)
        {
            vgets(temp, 255, f);
        }
        // suppress trailing CR/LF
        char *p = temp;
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

    void AddFollower()
    {
        ::Log::Write("AddFollower disabled.");

        ResolveOperand();       // entity

        /*
        int n = ResolveOperand();
        if (n<0 || n>=entities)
        {
            Sys_Error("AddFollower: Not a valid entity index. (%d)", n);
        }

        follower[(int)numfollowers++]=n;
        ResetFollowers();
        */
    }

    void KillFollower()
    {
        ResolveOperand();
    }

    void KillAllFollowers()
    {
    }

    void ResetFollowers()
    {
    }

    void FlatPoly()
    {
        int a, b, c, d, e, f, g;

        a = ResolveOperand();     // a
        b = ResolveOperand();     // b
        c = ResolveOperand();     // c
        d = ResolveOperand();     // d
        e = ResolveOperand();     // e
        f = ResolveOperand();     // f
        g = ResolveOperand();     // g

        gfx.FlatPoly(a, b, c, d, e, f, g);
    }

    void TMapPoly()
    {
        int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o;

        a = ResolveOperand();
        b = ResolveOperand();
        c = ResolveOperand();
        d = ResolveOperand();
        e = ResolveOperand();
        f = ResolveOperand();
        g = ResolveOperand();
        h = ResolveOperand();
        i = ResolveOperand();
        j = ResolveOperand();
        k = ResolveOperand();
        l = ResolveOperand();
        m = ResolveOperand();
        n = ResolveOperand();
        o = ResolveOperand();
        gfx.TMapPoly(a, b, c, d, e, f, g, h, i, j, k, l, m, n, (u8 *) o);
    }

    void CacheSound()
    {
        std::string     filename;

        filename        =ResolveString();

        vcreturn=::CacheSound(filename.c_str());
    }

    void PlaySound()
    {
        int             slot, volume, pan;

        slot    =ResolveOperand();
        volume  =ResolveOperand();
        pan             =ResolveOperand();

        PlaySFX(slot, volume, pan);
    }

    void RotScale()
    {
        int             x, y, width, length, angle, scale;
        int             ptr;

        x               = ResolveOperand();
        y               = ResolveOperand();
        width   = ResolveOperand();
        length  = ResolveOperand();
        angle   = ResolveOperand();
        scale   = ResolveOperand();
        ptr             = ResolveOperand();

        if (!lucentmode)
            gfx.RotScale(x, y, width, length, (float)(angle*3.14159/180.0), (float)(scale/1000.0f), (u8*)ptr);
        else
            gfx.RotScaleLucent(x, y, width, length, (float)(angle*3.14159/180.0), (float)(scale/1000.0f), (u8*)ptr, lucentmode);
    }

    namespace
    {
        class OpaqueBlit
        {
        public:
            inline void operator ()(int x, int y, int width, int height, u8* ptr) const
            {
                gfx.CopySprite(x, y, width, height, ptr);
            }
        };

        class TBlit
        {
        public:
            inline void operator ()(int x, int y, int width, int height, u8* ptr) const
            {
                gfx.TCopySprite(x, y, width, height, ptr);
            }
        };

        template <class T>
        void MapLine(int x, int y, int yoffset, int lay, const T& Blit)
        // blah.  poo.
        // The graphics routines are separate from the rest of VERGE, so it's all gotta be here. :P
        // I love templates, by the way. <:D
        {
            if (lay < 0 || lay >= map->NumLayers()) return;  // validate arguments
            //if (!layertoggle[lay])           return;  // is this layer visible?

            MapFile::TileLayer& layer = map->GetLayer(lay);

            // This is the location of the first tile we draw, it'll be at the left hand edge of the screen
            int x_map = (int)((xwin + x)       * layer.parx);
            int y_map = (int)((ywin + yoffset) * layer.pary);

            yoffset&=15;

            if (x_map < 0) x_map = 0;                 // make my life easier; don't allow scrolling past map edges
            if (y_map < 0) y_map = 0;

            int x_sub = -(x_map & 15);                // get subtile position while we still have pixel precision
            int y_sub =  (y_map & 15);

            x_map >>= 4;                              // determine upper left tile coords of camera
            y_map >>= 4;

            MapFile::TileLayer::Iterator iter(layer, x_map, y_map);
            // TODO: GetTile function (inline!)
            x = x_sub;

            do
            {
                int t=*iter;
                if (t)
                    //Blit(x, y, 16, 1, (u8*)vsp + (16*16* (*source) + (16*y_sub))*gfx.bpp);
                    Blit(x, y, 16, 1, (u8*)vsp->GetTile(t));

                //source += 1;
                iter++;
                x += 16;
            }
            while (x<gfx.XRes());
        }
    }

    void MapLine()
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

        MapLine(x, y, y_offset, lay, OpaqueBlit());
    }

    void TMapLine()
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

        MapLine(x, y, y_offset, lay, TBlit());
    }

    void val()
    {
        std::string     s;

        s       =ResolveString();

        vcreturn = atoi(s.c_str());
    }

    void TScaleSprite()
    {
        int             x, y, source_width, source_length, dest_width, dest_length;
        int             ptr;

        x                               =ResolveOperand();
        y                               =ResolveOperand();
        source_width    =ResolveOperand();
        source_length   =ResolveOperand();
        dest_width              =ResolveOperand();
        dest_length             =ResolveOperand();
        ptr                             =ResolveOperand();

        if (lucentmode)
            gfx.TScaleSpriteLucent(x, y, source_width, source_length, dest_width, dest_length, (u8 *)ptr,
            lucentmode);
        else
            gfx.TScaleSprite(x, y, source_width, source_length, dest_width, dest_length, (u8 *)ptr);
    }

    void GrabRegion()
    {
        int             x, y, xe, ye, bogus;

        x       =ResolveOperand();
        y       =ResolveOperand();
        xe      =ResolveOperand();
        ye      =ResolveOperand();

        bogus = 0;

        // ensure arguments stay valid
        if (x<0)
            x = 0,        bogus++;
        else if (x>=gfx.scrx)
            x = gfx.scrx-1,       bogus++;
        if (y<0)
            y = 0,        bogus++;
        else if (y>=gfx.scry)
            y = gfx.scry-1,       bogus++;

        if (xe<0)
            xe = 0,       bogus++;
        else if (xe>=gfx.scrx)
            xe = gfx.scrx-1, bogus++;
        if (ye<0)
            ye = 0,       bogus++;
        else if (ye>=gfx.scry)
            ye = gfx.scry-1, bogus++;

        if (bogus)
            ::Log::Write(va("GrabRegion: %d bogus args", bogus));

        // swap?
        if (xe<x) {
            int t = x;
            x = xe;
            xe = t;
        }
        if (ye<y) {
            int t = ye;
            y = ye;
            ye = t;
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

    void Log()
    {
        std::string message(ResolveString());

        ::Log::Write(message.c_str());
    }

    void fseekline()
    {
        int             line;
        VFILE*  vf;

        line    = ResolveOperand()-1;                                            // winv2 behaviour.  Subtract 2 for v2k+j behaviour
        vf      = GetReadFilePtr(ResolveOperand());

        vseek(vf, 0, SEEK_SET);

        // 1 will yield first line
        char temp[256+1];
        //      do
        while (line-->0)
        {
            vgets(temp, 256, vf);
        }// while (--line > 0);
    }

    void fseekpos()
    {
        int             pos;
        VFILE*  vf;

        pos     =ResolveOperand();
        vf      =GetReadFilePtr(ResolveOperand());

        vseek(vf, pos, 0);
    }

    void fread()
    {
        char*   buffer;
        int             len;
        VFILE*  vf;

        buffer  =(char *)ResolveOperand();
        len             =ResolveOperand();
        vf              =GetReadFilePtr(ResolveOperand());

        vread(buffer, len, vf);
    }

    void fgetbyte()
    {
        VFILE*  vf      =0;
        u8      b       =0;

        vf = GetReadFilePtr(ResolveOperand());
        vread(&b, 1, vf);

        vcreturn = b;
    }

    void fgetword()
    {
        VFILE*  vf      =0;
        u16     w       =0;

        vf = GetReadFilePtr(ResolveOperand());
        vread(&w, 2, vf);

        vcreturn = w;
    }

    void fgetquad()
    {
        VFILE*  vf      =0;
        u32             q       =0;

        vf = GetReadFilePtr(ResolveOperand());
        vread(&q, 4, vf);

        vcreturn = q;
    }

    void fgetline()
    {
        char    temp[256 +1];
        char*   p;
        int             code, offset;
        VFILE*  vf;

        // which global vc string do we read into?
        code = GrabC();
        offset = GrabW();
        if (op_SARRAY == code)
        {
            offset+=ResolveOperand();
        }

        // file pointer; blegh
        vf = GetReadFilePtr(ResolveOperand());

        // read line into temp buffer
        vgets(temp, 256, vf);
        temp[256]='\0';

        // suppress trailing CR/LF
        p = temp;
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

    void fgettoken()
    {
        char    temp[1024];
        int             code, offset;
        VFILE*  vf;

        // which global vc string do we read into?
        code = GrabC();
        offset = GrabW();
        if (code == op_SARRAY)
        {
            offset+=ResolveOperand();
        }

        // file pointer; blegh
        vf = GetReadFilePtr(ResolveOperand());

        // read into temp buffer
        vscanf(vf, "%s", temp);

        // assign to vc string
        if (offset>=0 && offset<stralloc)
        {
            vc_strings[offset]=temp;
        }
    }

    void fwritestring()
    {
        FILE*   f;
        std::string     temp;

        temp    =ResolveString();
        f = GetWriteFilePtr(ResolveOperand());

        fprintf(f, "%s\n", temp.c_str());
    }

    void fwrite()
    {
        char*   buffer;
        int             length;
        FILE*   f;

        buffer  =(char *)ResolveOperand();
        length  =ResolveOperand();

        f = GetWriteFilePtr(ResolveOperand());

        ::fwrite(buffer, 1, length, f);
    }

    void frename()
    {
        std::string     a, b;

        a = ResolveString();
        b = ResolveString();
        a = EnforceNoDirectories(a);
        b = EnforceNoDirectories(b);

        rename(a.c_str(), b.c_str());

        ::Log::Write(va(" --> VC renamed %s to %s.", a.c_str(), b.c_str()));
    }

    void fdelete()
    {
        std::string     filename;

        filename = ResolveString();
        filename = EnforceNoDirectories(filename);

        remove(filename.c_str());

        ::Log::Write(va(" --> VC deleted %s.", filename.c_str()));
    }

    void fwopen()
    {
        std::string     filename;

        filename        =ResolveString();
        filename        =EnforceNoDirectories(filename);

        vcreturn = OpenWriteVCFile(filename.c_str());

        ::Log::Write(va(" --> VC opened %s for writing, handle %i.", filename.c_str(), vcreturn));
    }

    void fwclose()
    {
        int idx = ResolveOperand();
        CloseVCFile(idx);

        ::Log::Write(va(" --> VC close file opened for writing, handle %i.", idx));
    }

    void memcpy()
    {
        char    *source, *dest;
        int             length;

        dest    =(char *)ResolveOperand();
        source  =(char *)ResolveOperand();
        length  =ResolveOperand();

        ::memcpy(dest, source, length);
    }

    void memset()
    {
        char*   dest;
        int             color, length;

        dest    =(char *)ResolveOperand();
        color   =ResolveOperand();
        length  =ResolveOperand();

        ::memset(dest, color, length);
    }

    // <aen, may 5>
    // + modified to use new silhouette vdriver routines
    // + added checks for ClipOn
    void Silhouette()
    {
        int width, height, src, dest, colour;
        width  = ResolveOperand();
        height = ResolveOperand();
        src    = ResolveOperand();
        dest   = ResolveOperand();
        colour = ResolveOperand();

        gfx.Silhouette(width, height, (u8*)src, (u8*)dest, colour);
    }

    void InitMosaicTable()
    {
    }

    void Mosaic()
    {
        ::Log::Write("Mosaic disabled.");

        ResolveOperand();       // a
        ResolveOperand();       // b
        ResolveOperand();       // c
        ResolveOperand();       // d
        ResolveOperand();       // e
        ResolveOperand();       // f
        ResolveOperand();       // g

        /*
        int a, b, c, d, e, f, g;

        a = ResolveOperand();
        b = ResolveOperand();
        c = ResolveOperand();
        d = ResolveOperand();
        e = ResolveOperand();
        f = ResolveOperand();
        g = ResolveOperand();
        Mosaic(a, b, (u8 *) c, d, e, f, g);
        */
    }

    void WriteVars()
    {
        FILE*   f;

        f = GetWriteFilePtr(ResolveOperand());

        for (int i = 0; i<maxint; i++)
            ::fwrite(&globalint[i], 1, 4, f);

        for (int n = 0; n<stralloc; n++)
        {
            int z = vc_strings[n].length();

            ::fwrite(&z, 1, 4, f);
            ::fwrite(vc_strings[n].c_str(), 1, z, f);
        }
    }

    void ReadVars()
    {
        VFILE*  f;

        f = GetReadFilePtr(ResolveOperand());

        for (int i = 0; i<maxint; i++)
            vread(&globalint[i], 4, f);

        for (int n = 0; n<stralloc; n++)
        {
            int z;
            vread(&z, 4, f);

            char* temp = new char[z+1];
            if (!temp)
                Sys_Error("Readars: memory exhausted on %d bytes.", z);
            vread(temp, z, f);
            temp[z]='\0';
            vc_strings[n]=temp;

            delete[] temp;
            temp = 0;
        }
    }

    void CallEvent()
    {
        ExecuteEvent(ResolveOperand());
    }

    void Asc()
    {
        vcreturn = ResolveString()[0];
    }

    void CallScript()
    {
        ExecuteUserFunc(ResolveOperand());
    }

    void NumForScript()
    {
        vcreturn = GrabD();
    }

    void FileSize()
    {
        std::string     filename;
        VFILE*  vf;

        filename        =ResolveString();

        vf = vopen(filename.c_str());
        vcreturn = filesize(vf);
        vclose(vf);
    }

    void ftell()
    {
        VFILE*  vf;

        vf = GetReadFilePtr(ResolveOperand());

        vcreturn = vtell(vf);
    }

    void CheckCorrupt()
    {
        //      ::Log::Write("CheckCorrupt disabled.");

        //*
        ::Log::Write("checking for corruption...");
        CheckCorruption();
        //*/
    }

    void ChangeCHR()
    {
        int             who;
        std::string     chrname;

        who = ResolveOperand();
        chrname = ResolveString();

        ::ChangeCHR(who, chrname.c_str());
    }

    void RGB()
    {
        int r, g, b;

        r = ResolveOperand();
        g = ResolveOperand();
        b = ResolveOperand();

        vcreturn
            = gfx.PackPixel(r, g, b);
    }

    // Gah!  replace these with unpackpixel! --tSB
    void GetR()
    {
        int color;
        int r, g, b;

        color = ResolveOperand();
        gfx.UnPackPixel(color, r, g, b);

        vcreturn = r;
    }

    void GetG()
    {
        int color;
        int r, g, b;

        color = ResolveOperand();
        gfx.UnPackPixel(color, r, g, b);

        vcreturn = g;
    }

    void GetB()
    {
        int color;
        int r, g, b;

        color = ResolveOperand();
        gfx.UnPackPixel(color, r, g, b);

        vcreturn = b;
    }

    void Mask()
    {
        int             source, mask, width, length, dest;

        source = ResolveOperand();
        mask = ResolveOperand();
        width = ResolveOperand();
        length = ResolveOperand();
        dest = ResolveOperand();

        gfx.Mask((u8*)source, (u8*)dest, width, length, (u8*)mask);
    }

    void ChangeAll()
    {
        int             source, width, length, source_color, dest_color;

        source = ResolveOperand();
        width = ResolveOperand();
        length = ResolveOperand();
        source_color = ResolveOperand();
        dest_color = ResolveOperand();

        gfx.ChangeAll(width, length, (u8*)source, source_color, dest_color);
    }

    void sqrt()
    {
        vcreturn=(int)::sqrt((float)ResolveOperand());
    }

    //- tSB
    void fwritebyte()
    {
        char b;
        FILE* f;

        b=(char)ResolveOperand();

        f = GetWriteFilePtr(ResolveOperand());
        ::fwrite(&b, 1, 1, f);
    }

    void fwriteword()
    {
        u16 b;
        FILE* f;

        b=(u16)ResolveOperand();

        f = GetWriteFilePtr(ResolveOperand());
        ::fwrite(&b, 1, 2, f);
    }

    void fwritequad()
    {
        int b;
        FILE* f;

        b = ResolveOperand();

        f = GetWriteFilePtr(ResolveOperand());
        ::fwrite(&b, 1, 4, f);
    }

    void CalcLucent()
    {
        gfx.CalcLucentLUT(ResolveOperand());
    }

    void ImageSize()
    {
        int x1, x2, y1, y2;

        x1 = ResolveOperand();
        y1 = ResolveOperand();
        x2 = ResolveOperand();
        y2 = ResolveOperand();

        if (x1>x2)
        {
            int i = x1;
            x1 = x2;
            x2 = i;
        }
        if (y1>y2)
        {
            int i = y1;
            y1 = y2;
            y2 = i;
        }

        x2-=x1;
        y2-=y1;
        vcreturn = x2*y2*gfx.bpp;
    }

}
//- end

VCFunc vcfunctions[]=
{
    0,
    VC::Exit_,               VC::Message,
    VC::Malloc,              VC::Free,
    VC::pow,                 VC::LoadImage,
    VC::CopySprite,          VC::TCopySprite,
    Render,                  VC::ShowPage,
    VC::EntitySpawn,         VC::SetPlayer,
    VC::Map,                 VC::LoadFont,
    VC::PlayFLI,             VC::GotoXY,
    VC::PrintString,         VC::LoadRaw,
    VC::SetTile,             VC::AllowConsole,
    VC::ScaleSprite,         ProcessEntities,
    VC::UpdateControls,      VC::Unpress,
    VC::EntityMove,          VC::HLine,
    VC::VLine,               VC::Line,
    VC::Circle,              VC::CircleFill,
    VC::Rect,                VC::RectFill,
    VC::strlen,              VC::strcmp,
    VC::CDStop,              VC::CDPlay,
    VC::FontWidth,           VC::FontHeight,
    VC::SetPixel,            VC::GetPixel,
    VC::EntityOnScreen,      VC::Rand,
    VC::GetTile,             VC::HookRetrace,
    VC::HookTimer,           VC::SetResolution,
    VC::SetRString,          VC::SetClipRect,
    VC::SetRenderDest,       VC::RestoreRenderSettings,
    VC::PartyMove,           VC::sin,
    VC::cos,                 VC::tan,
    VC::ReadMouse,           VC::SetClipping,
    VC::SetLucent,           VC::WrapBlit,
    VC::TWrapBlit,           VC::SetMousePos,
    VC::HookKey,             VC::PlayMusic,
    StopMusic,               VC::PaletteMorph,
    VC::OpenFile,            VC::CloseFile,
    VC::QuickRead,           VC::AddFollower,
    VC::KillFollower,        VC::KillAllFollowers,
    VC::ResetFollowers,      VC::FlatPoly,
    VC::TMapPoly,            VC::CacheSound,
    FreeAllSounds,           VC::PlaySound,
    VC::RotScale,            VC::MapLine,
    VC::TMapLine,            VC::val,
    VC::TScaleSprite,        VC::GrabRegion,
    VC::Log,                 VC::fseekline,
    VC::fseekpos,            VC::fread,
    VC::fgetbyte,            VC::fgetword,
    VC::fgetquad,            VC::fgetline,
    VC::fgettoken,           VC::fwritestring,
    VC::fwrite,              VC::frename,
    VC::fdelete,             VC::fwopen,
    VC::fwclose,             VC::memcpy,
    VC::memset,              VC::Silhouette,
    VC::InitMosaicTable,     VC::Mosaic,
    VC::WriteVars,           VC::ReadVars,
    VC::CallEvent,           VC::Asc,
    VC::CallScript,          VC::NumForScript,
    VC::FileSize,            VC::ftell,
    VC::ChangeCHR,           VC::RGB,
    VC::GetR,                VC::GetG,
    VC::GetB,                VC::Mask,
    VC::ChangeAll,           VC::sqrt,
    VC::fwritebyte,          VC::fwriteword,
    VC::fwritequad,          VC::CalcLucent,
    VC::ImageSize
};
