/*
    A fabulous example of just how much I like OOPifying everything under the sun.

    Also.  I am evil, for I use goto.
*/

#include "log.h"
#include "misc.h"
#include "movescript.h"

#include <iostream>
using std::cout;
using std::endl;

MoveScript::Command MoveScript::NextCommand(const string_k& s,int& offset)
{
    struct Local
    {
        static int GetInt(const string_k& s,int& ofs)
        {
            // make this nonstatic if we ever start caring about threadsafety.
            static string_k sofar;

            sofar="";
        
        getchar:
            char c=s[ofs++];
            if (ofs>s.length())
                return atoi(sofar.c_str());
            
            if (c>='0' && c<='9')
            {
                sofar+=c;
                goto getchar;
            }
            
            ofs--;                            // reset the index so that the parser sees this character next time through the loop.
            return atoi(sofar.c_str());
        }
    };
   
    Command cmd;

getcommand:

    char c=s[offset++];
    if (c>='A' && c<='Z') c|=32;
    
    bool shouldgetarg=true;
    
    switch (c)
    {
    case ' ':   goto getcommand;                                           // whitespace; skip
        
    case 'u':   cmd.type=ct_moveup;             break;
    case 'd':   cmd.type=ct_movedown;           break;
    case 'l':   cmd.type=ct_moveleft;           break;
    case 'r':   cmd.type=ct_moveright;          break;
    case 's':   cmd.type=ct_setspeed;           break;
    case 'w':   cmd.type=ct_wait;               break;
    case 'c':   cmd.type=ct_callevent;          break;
    case 'b':   cmd.type=ct_loop;               return cmd;//shouldgetarg=false; break;
    case 'x':   cmd.type=ct_setx;               break;
    case 'y':   cmd.type=ct_sety;               break;
    case 'f':   cmd.type=ct_setdirection;       break;
    case 'z':   cmd.type=ct_setframe;           break;
    default:    cmd.type=ct_stop;               return cmd;//shouldgetarg=false; break;
    }

    if (shouldgetarg)
        cmd.arg=Local::GetInt(s,offset);

    return cmd;
}

MoveScript::MoveScript(const string_k& str)
{
    int offset=0;
    Command cmd;

    // parse till there's nothing left to parse. :D
    while ( (cmd=NextCommand(str,offset)).type != ct_stop)
        cmds.push_back(cmd);

    cmds.push_back(cmd);

#if 0
    // test code
    static const char* bleh[]= 
    {        
        "up","down","left","right",
        "speed","wait","callevent","loop",
        "set_x","set_y","setdirection","setframe",
        "end"
    };

    // test code: dump the script
    if (cmds.size())
    {
        Log::Write(va("Move script: %s",str.c_str()));
        for (int i=0; i<cmds.size(); i++)
            Log::Write(va("\t%s\t%i",bleh[(int)cmds[i].type],cmds[i].arg));
    }
#endif
}