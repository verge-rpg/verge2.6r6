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

  
    verge.c
    Copyright (C) 1998 Benjamin Eirich
    
      Portability:
      MSVC6   - Yessir.
      Anything else - Have fun. ;D
      QBasic  - hahahah.  That just cracks me up.
*/

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <tSB, Nov 7>
// + moved startup-ish stuff over to startup.cpp (might have been an icky thing to do, lots of externs! @_@)
// <tSB, Oct 30>
// + started porting to Win32
// <aen, apr 21>
// + changed Log() & Logp() to take variable args.
// + altered translucency lookup code a bit
// + cleaned up USER.CFG parsing
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include <stdio.h>
#include <stdarg.h> // va_*
#include "verge.h"

// gah!
extern void ParseAutoCFG();

// ================================= Objects =============================


GrDriver gfx;
Input input;
CFontController font;

// ================================= Data ====================================


char	logoutput	=1;		// Verbose debugging startup mode

// ================================= Code ====================================


// InitSystems moved to startup.cpp, where it can have access to Win32

void vmainloop()
{
    CheckHookTimer();
    while (timer_count > 0 && !vckill)
    {
        timer_count--;
        GameTick();
    }
    
    if (vckill)
    {
        FreeVSP();
        FreeMAP();
        FreeCHRList();
        vcsp = vcstack;
        vckill = 0;
        LoadMAP(startmap.c_str());
    }
    Render();
    gfx.ShowPage();
}

void CheckMessages()
{
    // Win95 can bite me.
    // mehehehe --tSB

    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            input.Update(
                event.key.keysym.sym,          // egads, that's a lot of dereferencing
                event.key.state==SDL_PRESSED
                );

            // bottom line quit when alt-X is pressed
            if (input.key[SCAN_LMENU] && input.key[SCAN_X])
                Sys_Error("");

            // bottom line screenshot if F11 is pressed
            if (event.key.keysym.sym==SDLK_F11 && event.key.state==SDL_PRESSED)
                ScreenShot();

            break;
           
        case SDL_QUIT:
            Sys_Error("");
            break;
        }
    }
}

int VMain ()
{
//    if (argc==2)
//        startmap = argv[1];
    
    console.Init();

    /*
    Console_Printf(va("VERGE System Version %s", VERSION));
    Console_Printf("Copyright (C)1998 vecna");
    Console_Printf("");
    */
        
    Log::Writen("Loading system VC");
    LoadSystemVC();
    Log::Write("...OK");

    // startmap override?
    if (startmap.length())
        LoadMAP(startmap.c_str());
    else
        RunSystemAutoexec();
    
    // if there is no starting map at this point, we're done.
    if (startmap.length() < 1)
        Sys_Error("");
    
    while (1)
    {
        CheckMessages();
        vmainloop();
    }
    return 0;
}
