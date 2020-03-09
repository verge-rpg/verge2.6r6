
#ifndef MOVESCRIPT_H
#define MOVESCRIPT_H

#include "vtypes.h"
#include <vector>

using std::vector;

class MoveScript
{
public:
    enum CommandType
    {
        ct_moveup,
        ct_movedown,
        ct_moveleft,
        ct_moveright,
        ct_setspeed,
        ct_wait,
        ct_callevent,
        ct_loop,
        ct_setx,
        ct_sety,
        ct_setdirection,
        ct_setframe,
        ct_stop,
    };

    struct Command
    {
        CommandType type;
        int arg;

        Command() : type(ct_stop), arg(0) {}
    };

private:
    vector<Command> cmds;

    Command NextCommand(const std::string& s, uint& offset);

public:
    MoveScript(){}
    MoveScript(const std::string& str);

    Command& operator [](uint idx);
};

#endif
