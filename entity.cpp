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
// |                       Entity Movement module                        |
// \---------------------------------------------------------------------/

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <andy, whenever>
// Almost completely rewritten.
// <andy, June 9>
// Rewrote the CHR stuff.  It's all being moved to sprite.cpp and sprite.h
// <tSB, whenever>
// changed lidle, ridle, etc.. into array idle[4].  Makes things simpler in more than one place. :P
// <zero, 5.6.98>
// + corrected oversight in movement script management by sticking a hack in
//   MoveScript().  Bug caused Fx commands to not work sometimes.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "verge.h"
#include "misc.h"

#include "Map/MapFile.h"

// ================================= Data ====================================

std::string chrlist[100];
int playeridx=-1;  // index of the player entity
std::vector<Entity> ents;
int entities = 0;

Sprite* chr[100]= {0};

int numchrs = 0;
std::vector<int> entidx;
u8 numentsonscreen;
u8 movesuccess;

#include <iostream>
using std::cout;
using std::endl;

// ================================= Code ====================================

inline void Entity::TweakTileCoords(int& tx, int& ty, Direction d)
{
    switch (d)
    {
    case up:    ty--;   break;
    case down:  ty++;   break;
    case left:  tx--;   break;
    case right: tx++;   break;
    }
}

void Entity::Process()
{
    if (movecount>0)
        Move();
    else
    {
        switch (movecode)
        {
        case mp_wander:     Wander();       break;
        case mp_wanderrect: WanderRect();   break;
        case mp_wanderzone: WanderZone();   break;
        case mp_scripted:   Scripted();     break;

        case mp_stopped:
            if (ismoving)
                Stop();

            break;
        }
    }

    Animate();
}

void Entity::Update()
{
    static const int inc[] = { 1000, 125, 250, 500, 1000, 2000, 4000, 8000 };

    if (speed < 0) speed = 0;
    if (speed > sizeof inc - 1) speed = sizeof inc - 1;

    speedcount += inc[speed];
    while (speedcount >= 1000)
    {
        Process();
        speedcount -= 1000;
    }
}

// Initiates a one tile step in the specified direction
void Entity::Move(Direction d)
{
    int tx = x/16;
    int ty = y/16;

    TweakTileCoords(tx, ty, d);

    // tx and ty are now the tile we wish to move to
    if (isobs && (ObstructionAt(tx, ty) || EntityObsAt(tx, ty)!=-1))
    {
        direction = d;
        Stop();
        return; // move failed.  Freak out.
    }

    if (direction!=d || !ismoving)
    {
        direction = d;
        SetAnimScript(direction);
    }

    ismoving = true;
    movecount = 16;
}

// Handles entity subtile movement.
void Entity::Move()
{
    if (ismoving)
        TweakTileCoords(x, y, direction);

    movecount--;
}

void Entity::Wander()
{
    if (stepcount == -1)    // pick a new direction
    {
        direction = (Direction)(rand() % 4);
        stepcount = step;
    }

    if (stepcount > 0)      // keep moving in the same direction
    {
        Move(direction);
        stepcount--;
    }
    else                    // stop for a moment
    {
        Stop();
        movecount = delay;  // By setting movecount, we ensure that Wander() will not be called until the rest period is over.
        stepcount = -1;     // We then set stepcount to -1 to signify that a new direction of movement should be chosen.
    }
}

void Entity::WanderZone()
{
    Wander();

    if (!ismoving)
        return;

    int tx = x / 16;
    int ty = y / 16;
    TweakTileCoords(tx, ty, direction);

    if (map->Zone().Get(tx, ty) != wanderzone)
        Stop();
}

void Entity::WanderRect()
{
    Wander();

    if (!ismoving)
        return;

    int tx = x / 16;
    int ty = y / 16;
    TweakTileCoords(tx, ty, direction);

    if (tx < wanderrect.left  ||
        ty < wanderrect.top   ||
        tx > wanderrect.right ||
        ty > wanderrect.bottom)
        Stop();
}

void Entity::Scripted()
{
    if (--scriptcount>0)
    {
        if (ismoving)
            Move(direction);    // move another step in the same direction
        return;
    }

    MoveScript::Command cmd = curmovescript[scriptofs];
    scriptofs++;

    switch (cmd.type)
    {
    case MoveScript::ct_moveup:         scriptcount = cmd.arg;    Move(up);       break;
    case MoveScript::ct_movedown:       scriptcount = cmd.arg;    Move(down);     break;
    case MoveScript::ct_moveleft:       scriptcount = cmd.arg;    Move(left);     break;
    case MoveScript::ct_moveright:      scriptcount = cmd.arg;    Move(right);    break;
    case MoveScript::ct_setspeed:       speed = cmd.arg;                          break;
    case MoveScript::ct_wait:           Stop(); scriptcount = cmd.arg;            break;
    case MoveScript::ct_callevent:      ExecuteEvent(cmd.arg);                  break;
    case MoveScript::ct_loop:           scriptofs = 0;                            break;
    case MoveScript::ct_setx:
        if (x > cmd.arg)
        {
            scriptcount = (x - cmd.arg) / 16;
            Move(left);
            break;
        }
        else if (x < cmd.arg)
        {
            scriptcount = (cmd.arg - x) / 16;
            Move(right);
            break;
        }
        Stop();
        scriptcount = 1;
        break;

    case MoveScript::ct_sety:
        if (y > cmd.arg)
        {
            scriptcount = (y - cmd.arg) / 16;
            Move(left);
            break;
        }
        else if (y < cmd.arg)
        {
            scriptcount = (cmd.arg - y) / 16;
            Move(right);
            break;
        }
        Stop();
        scriptcount = 1;
        break;

    case MoveScript::ct_setdirection:   direction = (Direction)cmd.arg;     break;
    case MoveScript::ct_setframe:       specframe = cmd.arg;                break;
    case MoveScript::ct_stop:           Stop();     movecode = mp_stopped;  movescriptidx = -1; break;
    }
}

void Entity::Stop()
{
    animscriptidx=-1;
    animcount = 0;
    animofs = 0;

    ismoving = false;
    movecount = 0;

    curframe = chr[chrindex]->IdleFrame(direction);
}

// updates the entity's animation thing
void Entity::Animate()
{
    if (animcount==0)   return;

    if (--animcount>0)
        return;

    // Figure out what to do next

    // just in case there is no anim script set:
    if (!curanim.length())  return;

    struct Local
    {
        static int GetInt(std::string& s, unsigned int& p)
        {
            // make this nonstatic if we ever start caring about threadsafety.
            static std::string sofar;

            sofar="";
            do
            {
                char c = s[p++];
                if (p > s.length())
                    return atoi(sofar.c_str());

                if (c>='0' && c<='9')
                {
                    sofar+=c;
                    continue;
                }

                p--;                            // reset the index so that the parser sees this character next time through the loop.
                return atoi(sofar.c_str());
            } while (true);
        }
    };

getachar:

    if (animofs >= curanim.length())
        animofs = 0;  // wrap-around

    char c = curanim[animofs++];

    switch (c)
    {
    case ' ':                           // whitespace; skip
        goto getachar;
        // <:D --andy

    case 'F':
    case 'f':                           // v2 isn't case insensitive in this context, so this is technically breaking compatibility
        curframe = Local::GetInt(curanim, animofs);
        animcount = 1;
        break;

    case 'W':
    case 'w':
        animcount = Local::GetInt(curanim, animofs);
        break;
    }
}

void Entity::SetAnimScript(Direction d)
{
    animscriptidx=(int)d;
    curanim = chr[chrindex]->GetAnim(animscriptidx);
    animcount = 1;
    animofs = 0;
}

void Entity::SetMoveScript(const MoveScript& m)
{
    scriptofs = scriptcount = 0;
    curmovescript = m;
}

VFILE* operator >> (VFILE* f, Entity& e)
{
    e.x = vgetq(f)*16;
    e.y = vgetq(f)*16;
    vgetw(f);   vgetw(f);

    vgetc(f);   // face direction and move direction are one and the same
    e.direction=(Direction)vgetc(f);
    e.movecount = vgetc(f);
    e.curframe = vgetc(f);
    e.specframe = vgetc(f);
    e.chrindex = vgetc(f);
    // reset=
    vgetc(f); // ???

    e.isobs  = vgetc(f) != 0;
    e.canobs = vgetc(f) != 0;

    e.speed = vgetc(f);
    e.speedcount = vgetc(f);

    vgetc(f); // animation-frame delay (???)
    e.animscriptidx= vgetq(f);
    e.movescriptidx= vgetq(f);
    e.autoface     = vgetc(f) != 0;
    e.adjactivate  = vgetc(f) != 0;
    e.movecode     = (Entity::MovePattern)vgetc(f);
    e.movescript   = vgetc(f);
    vgetc(f); // subtile move count thing
    vgetc(f); // internal mode thing

    e.step = vgetw(f);
    e.delay = vgetw(f);
    e.stepcount = vgetw(f);
    e.delaycount = vgetw(f);

    vgetw(f); // data1
    e.wanderzone = e.wanderrect.right = vgetw(f);   // data2
    e.wanderrect.top   = vgetw(f);
    vgetw(f); // data4
    e.wanderrect.left  = vgetw(f);
    e.wanderrect.bottom= vgetw(f);
    vgetw(f); // ???
    e.actscript = vgetw(f);

    e.visible = e.on = true;

    char c[20];
    vread(c, 18, f);  // ??? skip

    vread(c, 20, f);
    c[19]=0;
    e.description = c;

    return f;
}

//------------------------ OLD POO -------------------------

int ObstructionAt(int tx, int ty)
{
    if (tx < 0 || ty < 0 || tx >= map->Width() || ty >= map->Height())
        return 1;

    return map->Obs().Get(tx, ty)?1:0;
}

int CacheCHR(const std::string& fname)
{
    if (numchrs >= 100)
        Sys_Error("CacheCHR: too many chrs loaded: %d", numchrs);

#ifdef WIN32
    // in win32, we lowercaseize all filenames for easier comparison later down the line
    chr[numchrs]=new Sprite(gfx, lowerCase(fname).c_str());
    chrlist[numchrs]=lowerCase(fname);
#else
    chr[numchrs]=new Sprite(gfx, fname.c_str());
    chrlist[numchrs]=fname;
#endif

    cout << "CacheCHR: " << fname.c_str() << " in slot #" << (int)numchrs << endl;

    return numchrs++;
}

void FreeCHRList()
{
    for (int n = 0; n<numchrs; n++)
    {
        chrlist[n]="";
        delete chr[n];
        chr[n]=0;
    }
    numchrs = 0;
}

void LoadCHRList(const std::vector<std::string>& l)
{
    FreeCHRList();
    for (unsigned int n = 0; n<l.size(); n++)
        if (l[n].length())
            CacheCHR(l[n]);
}

int FindCHR(const std::string& fname)
{
    int		n	=0;

    for (n = 0; n<numchrs; n++)
    {
#ifdef WIN32
        if (chrlist[n] == lowerCase(fname))
#else
        if (chrlist[n] == fname)
#endif
            return n;
    }

    return -1;
}

int EntityAt(int ex, int ey)
{
    for (int i = 0; i<entities; i++)
    {
        const Entity& e = ents[i];

        if (e.x   >=ex*16+16)    continue;
        if (e.y   >=ey*16+16)    continue;
        if (e.x+16<=ex*16)       continue;
        if (e.y+16<=ey*16)       continue;
        return i;
    }

    return -1;
}

int EntityObsAt(int ex, int ey)
{
    for (int i = 0; i<entities; i++)
    {
        const Entity& e = ents[i];

        if (!e.isobs)            continue;

        if (e.x   >=ex*16+16)    continue;
        if (e.y   >=ey*16+16)    continue;
        if (e.x+16<=ex*16)       continue;
        if (e.y+16<=ey*16)       continue;
        return i;
    }

    return -1;
}

void ProcessEntities()
{
    for (int n = 0; n < entities; n++)
    {
        if (n != playeridx)   // hack blech.
            ents[n].Update();
    }
}


int AllocateEntity(int x1, int y1, const char *fname)
{
    int n = FindCHR(fname);
    if (n==-1)
        n = CacheCHR(fname);

    Entity& e = ents[entities];

    e.Clear();

    e.chrindex = n;

    e.x = x1*16;
    e.y = y1*16;
    e.speed = 4;
    e.isobs = 1;
    e.canobs = 1;
    e.on = 1;
    e.visible = 1;
    e.curframe = chr[n]->IdleFrame(0);

    return entities++;
}

void ChangeCHR(uint who, const char* chrname)
{
    if (who<0 || who>=ents.size())
        Sys_Error("ChangeCHR: no such entity: %d", who);

    int n = FindCHR(chrname);
    if (n==-1)
        n = CacheCHR(chrname);

    ents[who].chrindex = n;
}
