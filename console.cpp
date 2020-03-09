/*
 * Copyright (C) 1998 BJ Eirich (aka vecna)
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// /---------------------------------------------------------------------\
// |                          The VERGE Engine                           |
// |         Copyright (C)1998 BJ Eirich (aka vecna) and others          |
// |                      Command Console module                         |
// \---------------------------------------------------------------------/

/*
mod log:

  <andy>
  12  December    2002    Rewritten from scratch.
  <tSB>
  12  Nov         2000    Rehashed the keyboard handler.  The console now actively pulls keys from the handler, instead
  of the handler "pushing" them around.
  <aen>
  28	Decemeber	1999	Major revamp.
*/

#include "SDL.h"
#include "corona.h"

#include "Console.h"
#include "verge.h" // egh.
#include "font.h"
#include "timer.h" // egh
#include "misc.h"
#include "a_memory.h"

Console::Console()
    : yposition(0)
    , direction(0)
    , hFont(0)
{
    // TODO: init functions here
    functions["ver"] = &Console::Ver;
    functions["clear"] = &Console::Clear;
    functions["exit"] = &Console::Exit;
    functions["help"] = &Console::Help;
    functions["setbackground"] = &Console::SetBackground;
    functions["cpuinfo"] = &Console::CPUInfo;

    Ver(StringVec());
}

Console::~Console()
{
    if (background.pixels)
        vfree(background.pixels);
}

void Console::Draw()
{
    yposition += direction * 2;

    if (yposition < 0)
    {
        direction = 0;
        yposition = 0;
        return;
    }

    if (yposition > gfx.YRes() / 2)
    {
        yposition = gfx.YRes() / 2;
        direction = 0;
        return;
    }

    if (yposition == 0)
        return;

    if (background.pixels)
    {
        gfx.ScaleSprite(0
            , yposition - background.height
            , background.width
            , background.height
            , gfx.XRes()
            , gfx.YRes() / 2
            , background.pixels);
    }
    else
        gfx.RectFill(0, 0, gfx.XRes(), yposition, 0, 0);

    int h = font[hFont].Height();

    int linestodraw = yposition / h;

    int y = yposition - h - h;    // go up one line, to make room for the current text buffer
    StringList::reverse_iterator iter = output.rbegin();
    for (int i = 0; i < linestodraw && iter != output.rend(); i++, iter++)
    {
        font.GotoXY(0, y);
        font.PrintString(hFont, iter->c_str());  // TODO: line wrap
        y -= font[hFont].Height();
    }

    y = yposition - h;
    font.GotoXY(0, y);
    font.PrintString(hFont, (std::string("> ") + curcommand).c_str());
    if (systemtime % 100 < 50)
        font.PrintString(hFont, "-");
}

void Console::SendKey(char c)
{
    switch (c)
    {
    case 8:
        if (curcommand.length() > 0)
            curcommand.erase(curcommand.length() - 1);
        return;

    case '\t':
        curcommand = TabComplete(curcommand);
        return;

    case 13:
        {
            Exec(curcommand);

            // TODO: add to command history
            curcommand.erase(0);
            return;
        }

    case '`':
        Close();
        return;
    // TODO: page up/down
    }

    if (c>=32 && c<=128)
        curcommand+=c;
}

void Console::Exec(const std::string& command)
{
    if (command.length() == 0)
        return;


    std::vector<std::string> cmd = splitString(command);
    if (cmd.size() > 0)
    {
        std::string commandname = lowerCase(cmd[0]);
        cmd.erase(cmd.begin());

        if (functions.count(commandname))
        {
            Write(va("-> %s", command.c_str()));
            (this->*functions[commandname])(cmd);
        }
        else
        {
            Write(va("Unknown command \"%s\"", commandname.c_str()));
        }
    }
}

void Console::Open()
{
    direction = 1;
}

void Console::Close()
{
    direction =- 1;
}

bool Console::IsOpen()
{
    return !( (direction == -1) || (yposition == 0) );
}

void Console::Enable()
{
    enabled = true;
}

void Console::Disable()
{
    enabled = false;
}

void Console::Write(const char* msg)
{
    output.push_back(msg);

    while (output.size() > 100)
        output.pop_front();
}

std::string Console::TabComplete(const std::string& cmd)
{
    if (cmd.find(' ') != std::string::npos)
        return cmd; // no tab complete if there's a space in the current command.

    StringVec possibleCommands;
    for (FunctionMap::iterator i = functions.begin(); i != functions.end(); i++)
    {
        if (lowerCase(i->first.substr(0, cmd.length())) == lowerCase(cmd))
            possibleCommands.push_back(i->first);
    }

    if (possibleCommands.size() == 1)
        return possibleCommands[0];
    else
    {
        for (int i = 0; i < possibleCommands.size(); i++)
            Write(possibleCommands[i].c_str());
        return cmd;
    }
}

void Console::Ver(const StringVec& args)
{
    Write("VERGE version " VERSION);
    Write("Built on " __DATE__ " at " __TIME__);
#ifdef _MSVC
    Write(va("with MS Visual C++ %i", _MSC_VER));
#endif
}

void Console::Clear(const StringVec& args)
{
    output.clear();
}

void Console::Exit(const StringVec& args)
{
    Sys_Error("");
}

void Console::Help(const StringVec& args)
{
    Write("Valid console commands are:");
    for (FunctionMap::iterator iter = functions.begin(); iter != functions.end(); iter++)
    {
        Write(va("   %s", iter->first.c_str()));
    }
}

void Console::SetBackground(const StringVec& args)
{
    if (args.size() != 1)
    {
        Write("Syntax:");
        Write("   setbackground FileName.png");
        return;
    }

    u8* p = Image_LoadBuf(args[0].c_str());
    if (!p)
    {
        Write(va("Image file \"%s\" not found.", args[0].c_str()));
        return;
    }

    if (background.pixels)
        vfree(background.pixels);

    background.pixels = p;
    background.width = Image_Width();
    background.height = Image_Length();
}

void Console::CPUInfo(const StringVec& args)
{
    if (args.size() != 1)
        goto syntax;

    {
        std::string s = lowerCase(args[0]);
        if (s == "true")
            cpu_watch = 1;
        else if (s == "false")
            cpu_watch = 0;
        else
            goto syntax;
    }
    return;

syntax:
    // laaaaaaaaazy
    Write("Syntax:");
    Write("   cpuinfo {true|false}");
    return;
}
