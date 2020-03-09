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
// |                         Messaging module                            |
// \---------------------------------------------------------------------/

#include "verge.h"
#include "misc.h"
#include <list>

using std::list;

// DATA /////////////////////////////////////////////////////////////////////////////////////////////////

struct Message
{
    std::string text;
    u32 bestbefore; // :D

    Message(){}
    Message(const std::string& s, u32 t) : text(s), bestbefore(t){}
};

typedef list<Message> MsgList;

MsgList messages;

// -- cpu usage --

static int cputimer = 0, frames = 0;
static char runprf[6];

// -- final numbers --

static int fps = 0;
static char profile[6];

// CODE /////////////////////////////////////////////////////////////////////////////////////////////////

static void Message_CheckExpirations()
{
    for (MsgList::iterator i = messages.begin(); i!=messages.end(); i++)
    {
        if (systemtime > i->bestbefore)
        {
            MsgList::iterator j = i;
            i++;

            messages.erase(j);
        }
    }
}

void RenderGUI()
{

    Message_CheckExpirations();

    int x = 1;
    int y = 1;

    for (MsgList::iterator i = messages.begin(); i!=messages.end(); i++)
    {
        font.GotoXY(x, y);
        font.PrintString(0, i->text.c_str());
        y+=font[0].Height();
    }

    if (!cpu_watch) return;
    frames++;

    x = gfx.scrx -font[0].Width()*10 -4;
    y = gfx.scry -font[0].Height()*4 -4;

    font.GotoXY(x, y);
    font.PrintString(0, va("   FPS:%d\n", fps));
    font.PrintString(0, va("Render:%d\n", profile[1]));
    font.PrintString(0, va(" PFlip:%d\n", profile[2]));
    font.PrintString(0, va("   etc:%d\n", profile[0]));
}

void CPUTick()
{
    cputimer++;
    runprf[cpubyte]++;
    if (cputimer==100)
    {
        fps = frames;
        frames = 0;
        cputimer = 0;

        profile[0]=runprf[0]; runprf[0]=0;
        profile[1]=runprf[1]; runprf[1]=0;
        profile[2]=runprf[2]; runprf[2]=0;
    }
}

void Message_Send(std::string text, int duration)
{
    Log::Write(va("Message: %s", text.c_str()));

    messages.push_back(Message(text, duration));

    // make sure it's not too big
    if (messages.size()>5)
    {
        messages.pop_front();
    }
}
