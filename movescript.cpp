/*
    A fabulous example of just how much I like OOPifying everything under the sun.

    Also.  I am evil, for I use goto.  I've decided that it is necessary because
    C++ doesn't have the right constructs to make things intuitive.  In this case
    tail recursion is the better way.
*/

#include <cassert>
#include <iostream>

#include "log.h"
#include "misc.h"
#include "movescript.h"

using std::cout;
using std::endl;

MoveScript::Command MoveScript::NextCommand(const std::string& s, uint& offset)
{
    struct Local
    {
        static int GetInt(const std::string& s, uint& ofs)
        {
            // make this nonstatic if we ever start caring about threadsafety.
            static std::string sofar;

            sofar="";
        
        getchar:
            char c = s[ofs++];
            if (ofs > s.length())
                return atoi(sofar.c_str());
            
            if (c>='0' && c<='9')
            {
                sofar+=c;
                goto getchar;
            }
            
            ofs--;                          // reset the index so that the parser sees this character next time through the loop.
            return atoi(sofar.c_str());
        }
    };
   
    Command cmd;

getcommand:

    char c = s[offset++];
    if (c>='A' && c<='Z') c|=32;
    
    switch (c)
    {
    // yay goto.  If this offends you, pretend that it's actually:
    // return NextCommand(s, offset);
    case ' ':   goto getcommand;                            // whitespace; skip
        
    case 'u':   cmd.type = ct_moveup;             break;
    case 'd':   cmd.type = ct_movedown;           break;
    case 'l':   cmd.type = ct_moveleft;           break;
    case 'r':   cmd.type = ct_moveright;          break;
    case 's':   cmd.type = ct_setspeed;           break;
    case 'w':   cmd.type = ct_wait;               break;
    case 'c':   cmd.type = ct_callevent;          break;
    case 'b':   cmd.type = ct_loop;               return cmd;
    case 'x':   cmd.type = ct_setx;               break;
    case 'y':   cmd.type = ct_sety;               break;
    case 'f':   cmd.type = ct_setdirection;       break;
    case 'z':   cmd.type = ct_setframe;           break;
    default:    cmd.type = ct_stop;               return cmd;
    }

    // Commands that don't accept arguments don't reach this point; assume that the command needs a numeric argument.
    cmd.arg = Local::GetInt(s, offset);

    return cmd;
}

MoveScript::MoveScript(const std::string& str)
{
    uint offset = 0;
    Command cmd;

    // parse till there's nothing left to parse. :D
    while ( (cmd = NextCommand(str, offset)).type != ct_stop)
        cmds.push_back(cmd);

    cmds.push_back(cmd);

#if 0
    // test code
    static const char* bleh[]= 
    {        
        "up", "down", "left", "right",
        "speed", "wait", "callevent", "loop",
        "set_x", "set_y", "setdirection", "setframe",
        "end"
    };

    // test code: dump the script
    if (cmds.size())
    {
        Log::Write(va("Move script: %s", str.c_str()));
        for (int i = 0; i<cmds.size(); i++)
            Log::Write(va("\t%s\t%i", bleh[(int)cmds[i].type], cmds[i].arg));
    }
#endif
}

MoveScript::Command& MoveScript::operator [](uint idx)
{
    assert(idx >=0 && idx < cmds.size());
    return cmds[idx];
}
