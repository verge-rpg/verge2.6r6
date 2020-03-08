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

#ifndef CONSOLE_INC
#define CONSOLE_INC

#include "verge.h"
#include "vtypes.h"

#define ETC 0
#define RENDER 1
#define PFLIP 2

extern u8 cpu_watch;
extern u8 cpubyte;

#include <string>
#include <list>
#include <vector>
using std::list;
using std::vector;
using std::string;
#pragma warning (disable:4786)

#include "vtypes.h"


class CFont;


class Console
{
    typedef void (*ConsoleFunc)(vector<string_k>& args);

    struct ConsoleCommand
    {
        string name;            // the name of the command
        ConsoleFunc Execute;    // the function to call

        ConsoleCommand(const string& s,ConsoleFunc func) 
            : name(s),Execute(func)
        {}
    };

    list<string> lines;         // command history
    list<ConsoleCommand> cmds;  // known console commands

    CFont* font;                // console text font

    u8* imgptr;                 // background image pointer
    int imgx,imgy;              // background image dimensions

    string_k curline;           // what the user has typed so far

    bool kill;                  // kill-flag

    void Render();              // draws everything

public:
    Console();
    ~Console();

    void Init();
    void Activate();
    void Print(const string_k& str);

    void ExecuteCommand(const string_k& cmd);
};

#endif // CONSOLE_INC
