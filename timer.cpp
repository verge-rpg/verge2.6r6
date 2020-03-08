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
// ³                    Timer / PIC contoller module                     ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

// ChangeLog
// Rewritten again.  SDL
// <tSB>  Nov 4, '00 - rewritten to work in Win32.  Borrows heavily from vecna's winv1.

#include "verge.h"

// ================================= Data ====================================

u32 systemtime,timer_count,vctimer,hktimer;
SDL_TimerID timerid=0;

// ================================= Code ====================================

Uint32 TimeProc(Uint32 interval,void*)
{  
    //~ if (!bActive) return; // bleh
    systemtime++;
    timer_count++;
    hktimer++;
    vctimer++;
	
    if (cpu_watch)	CPUTick();             
//    CheckTileAnimation();
    // sfxUpdate();

    return interval;
}

int InitTimer()
{
    if (timerid)
        SDL_RemoveTimer(timerid);
    timerid=SDL_AddTimer(10,&TimeProc,0);

    return timerid!=0;
}

void ShutdownTimer()
{
    SDL_RemoveTimer(timerid);
    timerid=0;
}
