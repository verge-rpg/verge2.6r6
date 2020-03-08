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
// ³                      Command Console module                         ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

/*
mod log:

  <andy>
  6     June        2002    Rewrite

  <tSB>
  12    Nov         2000    Rehashed the keyboard handler.  The console now actively pulls keys from the handler, instead
                            of the handler "pushing" them around.
  <aen>
  28	Decemeber	1999	Major revamp.
*/

#include "console.h"
#include "image.h"

// these have no business being here.  Move them.
u8 cpu_watch	=0;
u8 cpubyte		=0;
//--

Console::Console()
{
    imgptr=0;
    imgx=imgy=0;
}

Console::~Console()
{
    if (imgptr)
        vfree(imgptr);
}

void Console::Init()
{
    imgptr=Image_LoadBuf("console.gif");
    imgx=Image_Width();
    imgy=Image_Length();

    font=&::font[::font.Load("system.fnt")];
}

void Console::Activate()
{
    // scroll console down
    kill=false;
    curline="";

    while (!kill)
    {
        Render();
        CheckMessages();

        char c=input.GetKey();
        if (!c) continue;

        c=input.Scan2ASCII(c);
        
        switch (c)
        {
        case 13:                    // enter key
            ExecuteCommand(curline);
            break;

        case 8:                     // backspace
            if (curline.length() > 0)
                curline=curline.left(curline.length()-1);
            break;

        case '`':
            return;

        default:                    // alphanumeric key
            curline+=c;
            break;
        }
    }
}

void Console::Render()
{
    ::Render();

    gfx.CopySprite(0,0,imgx,imgy,imgptr);
    // write text

    int x=0,y=0;
    font->Print(curline.c_str(),x,y,0);

    gfx.ShowPage();
}

void Console::ExecuteCommand(const string_k& cmd)
{
}