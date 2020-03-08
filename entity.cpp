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
// ³                 Entity and Player Movement module                   ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
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
#include "strk.h"

// ================================= Data ====================================

string_k chrlist[100];
//Entity* player=0;
int playeridx;  // index of the player entity
std::vector<Entity> ents;
int entities=0;

Sprite* chr[100]= {0};

int numchrs=0;
std::vector<int> entidx;
u8 cc;
u8 movesuccess;

#include <iostream>
using std::cout;
using std::endl;

// ================================= Code ====================================

void Entity::TweakTileCoords(int& tx,int& ty,Direction d)
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
            {
                Stop();
                cout << "Stopped\t" << x << "\t" << y << endl;
            }
            
            break;
        }
    }

    Animate();
}

// Initiates a one tile step in the specified direction
void Entity::Move(Direction d)
{
    int newx=x;
    int newy=y;

    int tx=x/16;
    int ty=y/16;
    
    TweakTileCoords(tx,ty,d);
   
    // tx and ty are now the tile we wish to move to
    if (isobs && (ObstructionAt(tx,ty) || EntityObsAt(tx,ty)!=-1))
    {
        direction=d;
        Stop();
        return; // move failed.  Freak out.
    }

    if (direction!=d || !ismoving)
    {
        direction=d;
        SetAnimScript(direction);
    }

    ismoving=true;
    movecount=16;
}

// Handles entity subtile movement.
void Entity::Move()
{
    TweakTileCoords(x,y,direction);

    movecount--;
}

void Entity::Wander()
{
    if (direction!=-1)
    {
        chr[chrindex]->IdleFrame(direction);
        movecount=delaycount;
    }

    direction=(Direction)(rand()%4);
}

void Entity::WanderZone()
{
    Wander();

    int tx=x/16;
    int ty=y/16;
    TweakTileCoords(tx,ty,direction);

    if (Zone(tx,ty)!=wanderzone)
        Stop();
}

void Entity::WanderRect()
{
    Wander();

    int tx=x/16;
    int ty=y/16;
    TweakTileCoords(tx,ty,direction);

    if (tx<wanderrect.left  ||
        ty<wanderrect.top   ||
        tx>wanderrect.right ||
        ty>wanderrect.bottom)
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

    MoveScript::Command cmd=curmovescript[scriptofs];
    scriptofs++;

    switch (cmd.type)
    {
        case MoveScript::ct_moveup:         scriptcount=cmd.arg;    Move(up);       break;
        case MoveScript::ct_movedown:       scriptcount=cmd.arg;    Move(down);     break;
        case MoveScript::ct_moveleft:       scriptcount=cmd.arg;    Move(left);     break;
        case MoveScript::ct_moveright:      scriptcount=cmd.arg;    Move(right);    break;
        case MoveScript::ct_setspeed:       speed=cmd.arg;                          break;
        case MoveScript::ct_wait:           Stop(); scriptcount=cmd.arg;            break;
        case MoveScript::ct_callevent:      ExecuteEvent(cmd.arg);                  break;
        case MoveScript::ct_loop:           scriptofs=0;                            break;
        case MoveScript::ct_setx:
            if (x>cmd.arg)
            {
                scriptcount=(x-cmd.arg)/16;
                Move(left);
                break;
            }
            else if (x<cmd.arg)
            {
                scriptcount=(cmd.arg-x)/16;
                Move(right);
                break;
            }
            Stop();
            scriptcount=1;
            break;

        case MoveScript::ct_sety:
            if (y>cmd.arg)
            {
                scriptcount=(y-cmd.arg)/16;
                Move(left);
                break;
            }
            else if (y<cmd.arg)
            {
                scriptcount=(cmd.arg-y)/16;
                Move(right);
                break;
            }
            Stop();
            scriptcount=1;
            break;

        case MoveScript::ct_setdirection:   direction=(Direction)cmd.arg;       break;
        case MoveScript::ct_setframe:       specframe=cmd.arg;                  break;
        case MoveScript::ct_stop:           Stop();     movecode=mp_stopped;    movescriptidx=-1;   break;
    }
}

void Entity::Stop()
{
    animscriptidx=-1;
    animcount=0;
    animofs=0;

    ismoving=false;
    movecount=0;

    curframe= chr[chrindex]->IdleFrame( direction );
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
        static int GetInt(string_k& s,int& p)
        {
            // make this nonstatic if we ever start caring about threadsafety.
            static string sofar;

            sofar="";
            do
            {
                char c=s[p++];
                if (p>s.length())
                    return atoi(sofar.c_str());

                if (c>='0' && c<='9')
                {
                    sofar+=c;
                    continue;                   // Maybe I should replace this with a goto as well.  I dunno.  This seems pretty gay.
                }

                p--;                            // reset the index so that the parser sees this character next time through the loop.
                return atoi(sofar.c_str());
            } while (true);
        }
    };

getachar:

    if (animofs>=curanim.length())
        animofs=0;  // wrap-around
    
    char c=curanim[animofs++];
    
    switch (c)
    {
    case ' ':                           // whitespace; skip
        goto getachar;
        // <:D --andy
        
    case 'F':
    case 'f':                           // v2 isn't case insensitive in this context, so this is technically breaking compatibility
        curframe=Local::GetInt(curanim,animofs);
        animcount=1;
        break;
        
    case 'W':
    case 'w':
        animcount=Local::GetInt(curanim,animofs);
        break;
    }
}

void Entity::SetAnimScript(Direction d)
{
    if (animscriptidx==(int)d)
        return;

    animscriptidx=(int)d;
    curanim=chr[chrindex]->GetAnim(animscriptidx);
    animcount=1;
    animofs=0;
}

VFILE* operator >> (VFILE* f,Entity& e)
{
    e.x=vgetq(f)*16;
    e.y=vgetq(f)*16;
    vgetw(f);   vgetw(f);

    vgetc(f);   // face direction and move direction are one and the same
    e.direction=(Direction)vgetc(f);
    e.movecount=vgetc(f);
    e.curframe=vgetc(f);
    e.specframe=vgetc(f);
    e.chrindex=vgetc(f);
    // reset=
    vgetc(f); // ???

    e.isobs  = vgetc(f) != 0;
    e.canobs = vgetc(f) != 0;

    e.speed=vgetc(f);
    e.speedcount=vgetc(f);

    vgetc(f); // animation-frame delay (???)
    e.animscriptidx= vgetq(f);
    e.movescriptidx= vgetq(f);
    e.autoface     = vgetc(f) != 0;
    e.adjactivate  = vgetc(f) != 0;
    e.movecode     = (Entity::MovePattern)vgetc(f);
    e.movescript   = vgetc(f);
    vgetc(f); // subtile move count thing
    vgetc(f); // internal mode thing

    e.step=vgetw(f);
    e.delay=vgetw(f);
    e.stepcount=vgetw(f);
    e.delaycount=vgetw(f);

    vgetw(f); // data1
    e.wanderzone = e.wanderrect.right = vgetw(f);   // data2
    e.wanderrect.top   = vgetw(f);
    vgetw(f); // data4
    e.wanderrect.left  = vgetw(f);
    e.wanderrect.bottom= vgetw(f);
    vgetw(f); // ???
    e.actscript=vgetw(f);

    e.visible=e.on=true;

    char c[20];
    vread(c,18,f);  // ??? skip

    vread(c,20,f);
    c[19]=0;
    e.description=c;

    return f;
}

//------------------------ OLD POO -------------------------

int ObstructionAt(int tx, int ty)
{
    if (tx<0 || tx>=layer[0].sizex || ty<0 || ty>=layer[0].sizey)
        return 1;
    
    return obstruct[(ty*layer[0].sizex)+tx];
}

int Zone(int tx, int ty)
{
    if (tx<0 || tx>=layer[0].sizex || ty<0 || ty>=layer[0].sizey)
        return 0;
    
    return zone[(ty*layer[0].sizex)+tx];
}

int CacheCHR(const string_k& fname)
{
    if (numchrs >= 100)
        Sys_Error("CacheCHR: too many chrs loaded: %d", numchrs);
    
#ifdef WIN32
    // in win32, we lowercaseize all filenames for easier comparison later down the line
    chr[numchrs]=new Sprite(gfx,fname.lower().c_str());
    chrlist[numchrs]=fname.lower();
#else
    chr[numchrs]=new Sprite(gfx,fname.c_str());
    chrlist[numchrs]=fname;
#endif

    cout << "CacheCHR: " << fname.c_str() << " in slot #" << (int)numchrs << endl;

    return numchrs++;
}

void FreeCHRList()
{
    for (int n=0; n<numchrs; n++)
    {
        chrlist[n]="";
        delete chr[n];
        chr[n]=0;
    }
    numchrs=0;
}

void LoadCHRList(const std::vector<string_k>& l)
{
    FreeCHRList();
    for (int n=0; n<l.size(); n++)
        if (l[n].length())
            CacheCHR(l[n]);
}

int FindCHR(const string_k& fname)
{
    int		n	=0;
    
    for (n=0; n<numchrs; n++)
    {
#ifdef WIN32
        if (chrlist[n] == fname.lower())
#else
        if (chrlist[n] == fname)
#endif
            return n;
    }
    
    return -1;
}

int EntityAt(int ex,int ey)
{
    for (int i=0; i<entities; i++)
    {
        const Entity& e=ents[i];

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
    for (int i=0; i<entities; i++)
    {
        const Entity& e=ents[i];

        if (!e.isobs)            continue;

        if (e.x   >=ex*16+16)    continue;
        if (e.y   >=ey*16+16)    continue;
        if (e.x+16<=ex*16)       continue;
        if (e.y+16<=ey*16)       continue;
        return i;
    }

    return -1;
}

/*void Whitespace(int i)
{
    while (' ' == *entity[i].scriptofs)
        entity[i].scriptofs++;
}

void GetArgMS(int i)
{
    int		j;
    char	token[10];
    
    j=0;
    Whitespace(i);
    while (*entity[i].scriptofs>='0' && *entity[i].scriptofs<='9')
    {
        token[j++]=*entity[i].scriptofs++;
    }
    token[j]='\0';
    entity[i].data1=(u16)atoi(token);
}

void GetNextCommandMS(int i)
{
    Whitespace(i);
    
    switch (*entity[i].scriptofs++)
    {
    case 'u':
    case 'U':
        entity[i].mode=1;
        GetArgMS(i);
        break;
    case 'd':
    case 'D':
        entity[i].mode=2;
        GetArgMS(i);
        break;
    case 'l':
    case 'L':
        entity[i].mode=3;
        GetArgMS(i);
        break;
    case 'r':
    case 'R':
        entity[i].mode=4;
        GetArgMS(i);
        break;
    case 's':
    case 'S':
        entity[i].mode=5;
        GetArgMS(i);
        break;
    case 'w':
    case 'W':
        entity[i].mode=6;
        GetArgMS(i);
        entity[i].animofs=0;
        entity[i].delayct=0;
        break;
        
    case 0:                               // End of script, set entity movement to "stopped"
        entity[i].animofs=0;
        entity[i].frame=(u8)chr[entity[i].chrindex]->IdleFrame(entity[i].facing); // set the appropriate idle frame
        entity[i].movecode=0;                                                 // movecode=0 (stopped)
        entity[i].mode=7;
        entity[i].data1=0;
        entity[i].scriptofs=0;
        entity[i].delayct=0;
        break;
        
    case 'c':
    case 'C':
        entity[i].mode=8;
        GetArgMS(i);
        break;
    case 'b':
    case 'B':
        entity[i].mode=9;
        break;
    case 'x':
    case 'X':
        entity[i].mode=10;
        GetArgMS(i);
        break;
    case 'y':
    case 'Y':
        entity[i].mode=11;
        GetArgMS(i);
        break;
    case 'f':
    case 'F':
        entity[i].mode=12;
        GetArgMS(i);
        break;
    case 'z':
    case 'Z':
        entity[i].mode=13;
        GetArgMS(i);
        break;
        
    default:
        Sys_Error("Invalid entity movement script.");
    }
}

void MoveScript(int i)
{
    if (!entity[i].scriptofs)
        entity[i].scriptofs = ms+(int)msbuf[entity[i].movescript];
    
    if (!entity[i].mode)
        GetNextCommandMS(i);
    
    switch (entity[i].mode)
    {
    case 1:
        MoveUp(i);
        if (movesuccess)
            entity[i].data1--;
        break;
    case 2:
        MoveDown(i);
        if (movesuccess)
            entity[i].data1--;
        break;
    case 3:
        MoveLeft(i);
        if (movesuccess)
            entity[i].data1--;
        break;
    case 4:
        MoveRight(i);
        if (movesuccess)
            entity[i].data1--;
        break;
    case 5:
        entity[i].speed=(u8)entity[i].data1;
        entity[i].data1=0;
        break;
    case 6:
        entity[i].data1--;
        break;
    case 7:
        return;
    case 8:
        lastent=i;
        ExecuteEvent(entity[i].data1);
        entity[i].data1=0;
        break;
    case 9:
        entity[i].scriptofs	=ms+(int)msbuf[entity[i].movescript];
        entity[i].data1=0;
        break;
    case 10:
        if (entity[i].tx<entity[i].data1)
            MoveRight(i);
        if (entity[i].tx>entity[i].data1)
            MoveLeft(i);
        if (entity[i].tx==entity[i].data1)
            entity[i].data1=0;
        break;
    case 11:
        if (entity[i].ty<entity[i].data1)
            MoveDown(i);
        if (entity[i].ty>entity[i].data1)
            MoveUp(i);
        if (entity[i].ty==entity[i].data1)
            entity[i].data1=0;
        break;
    case 12:
        EntitySetFace(i, entity[i].data1);
        entity[i].data1=0;
        break;
    case 13:
        entity[i].specframe=(u8)entity[i].data1;
        entity[i].data1=0;
        break;
    }
    
    if (!entity[i].data1)
    {
        entity[i].mode=0;
        //      entity[i].movecode=0; // tSB- 11.20.00
    }
}*/

void TestActive(int i)
{
    /*int		dx, dy;
    
    dx	=abs(entity[i].x - player->x);
    dy	=abs(entity[i].y - player->y);
    if ((dx<=16 && dy<=3) || (dx<=3 && dy<=16))
    {
        if (!entity[i].expand4 && !invc)
        {
            entity[i].expand4=1;
            ExecuteEvent(entity[i].actscript);
        }
    }
    else
        entity[i].expand4=0;*/
}

void SiftEntities();

void ProcessEntities()
{
    //SiftEntities(); // in case people still want to affect only onscreen ents (the hell? --andy)
    for (int n=0; n < entities; n++)
    {
        if (n!=playeridx)   // hack blech.
            ents[n].Process();
    }
}


int AllocateEntity(int x1, int y1, const char *fname)
{
    int n=FindCHR(fname);
    if (n==-1)
        n=CacheCHR(fname);

    Entity& e=ents[entities];

    e.Clear();

    e.chrindex=n;
    
    e.x=x1*16;
    e.y=y1*16;
    e.speed=4;
    e.isobs=1;
    e.canobs=1;
    e.on=1;
    e.visible=1;
    
    return entities++;
}

void ChangeCHR(int who, const char* chrname)
{
    if (who<0 || who>=ents.size())
        Sys_Error("ChangeCHR: no such entity: %d", who);
    
    int n=FindCHR(chrname);
    if (n==-1)
        n=CacheCHR(chrname);

    ents[who].chrindex=n;
}