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

#ifndef ENTITY_H
#define ENTITY_H

#include "sprite.h"
#include "movescript.h"
#include "vfile.h"
#include <vector>

using std::vector;

// new entity class (except that I'm too lazy to make accessors for all this junk)
struct Entity
{
    enum MovePattern
    {
        mp_stopped,
        mp_wander,
        mp_wanderrect,
        mp_wanderzone,
        mp_scripted,
    };

    int         x,y;                    // pixel-coords.  divide if you want tile coords.

    Direction   direction;
    int         movecount;              // number of ticks before the entity stops doing whatever its doing and does something else
    int         curframe;
    bool        ismoving;               // true if the entity is moving.  False if not.  Real simple. :)

    int         specframe;              // overrides normal animation frame if nonzero
    int         chrindex;               // handle to the CHR
    // int reset?

    bool        isobs;                  // can be obstructed
    bool        canobs;                 // can obstruct other ents
    int         speed;
    int         speedcount;
    //int         delaycount;

    int         animscriptidx;
    string_k    curanim;
    int         animofs;
    int         animcount;              // counter thingie

    int         movescriptidx;
    MoveScript  curmovescript;
    int         scriptofs;
    int         scriptcount;

    bool        autoface;
    bool        adjactivate;            // if true, the entity's actscript is run when the entity is adjacent to the player

    MovePattern movecode;
    int         movescript;

    //int         subtilecount;           // ???
    int         mode;                   // ???
    
    int         step;                   // wandering things
    int         delay;                  // ditto
    int         stepcount,delaycount;

    bool        visible;
    bool        on;                     // the entity essentially does not exist if this is false

    int         wanderzone;             // the zone the entity must stay on as it wanders (if applicable)
    SRect       wanderrect;             // the rect the entity must stay within as it wanders (if applicable)

    int         actscript;              // script called when the entity is activated

    string_k    description;            // arbitrary

    int tx() const { return x>>4;   }
    int ty() const { return y>>4;   }

    // interface
    void Process();

    Entity()
    {        
        Clear();
    }

    void Clear()
    {
        // :D
        x=y=movecount=curframe=specframe=chrindex=speed=speedcount=animscriptidx=animcount=animofs=movescriptidx=scriptofs=scriptcount=mode=step=delay=stepcount=delaycount=wanderzone=actscript=0;
        ismoving=adjactivate=false;
        isobs=canobs=autoface=visible=on=true;
        direction=down;
        movecode=mp_stopped;
    }

    void SetMoveScript(int idx,MoveScript& m)
    {
        if (idx!=movescriptidx)
            scriptofs=scriptcount=0;

        movescriptidx=idx;
        curmovescript=m;

    }

//private:

    static void TweakTileCoords(int& x,int& y,Direction d);
    void Move(Direction d);
    void Move();        // move one pixel in the current direction

    void Wander();      // picks a random direction.
    void WanderRect();  // picks a random direction, provided that it is within the rect.
    void WanderZone();  // stays on a given zone
    void Scripted();    // follows the script

    void Stop();        // stops!  And sets the idle frame.

    void Animate();
    void SetAnimScript(Direction d);
};

VFILE* operator >> (VFILE* f,Entity& e);

extern string_k chrlist[100];
extern int playeridx;                   // player index
extern std::vector<Entity> ents;
extern Sprite* chr[100];
extern int entities, numchrs;
extern std::vector<int> entidx;
extern u8 cc;

void ProcessEntities();
void LoadCHRList(const vector<string_k>& l);
void FreeCHRList();
//void EntityStat();
//void ListActiveEnts();
//void EntityS();
int EntityAt(int ex, int ey);
int EntityObsAt(int ex, int ey);

int ObstructionAt(int ex, int ey);
int Zone(int tx,int ty);
int CacheCHR(const string_k& fname);
int AllocateEntity(int x1, int y1, const char *fname);
void ChangeCHR(int who, const char* chrname);

#endif
