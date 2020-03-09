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
// |                          Rendering module                           |
// \---------------------------------------------------------------------/

#include "verge.h"
#include "misc.h"
#include "tileanim.h"
#include "engine.h"
#include "Map/MapFile.h"
#include "vsp.h"
#include "Console.h"
#include <math.h>
#include <algorithm>

using Map::MapFile;

// INTERFACE DATA //////////////////////////////////////////////////////////////////////////////////

unsigned char	animate		= 0;
unsigned char	cameratracking	= 1;
int	tracker		= 0;
unsigned char	showobs		= 1;
unsigned char	showzone	= 0;

// IMPLEMENTATION CODE /////////////////////////////////////////////////////////////////////////////

// -------------------------- Map ----------------------------

/*
template <typename T>
void RenderLayer(int lay, T DrawTile)
{
    int		c;
    int		x;
    int		y;
    int		x_sub;
    int		y_sub;
    int		clip_width;
    int		clip_length;
    unsigned short*	source;

    // validate arguments
    if (lay < 0 || lay >= numlayers)
        return;

    // is this layer visible?
    if (!layertoggle[lay])
        return;

    // adjust view according to parallax
    x = xwin*layer[lay].pmultx/layer[lay].pdivx;
    y = ywin*layer[lay].pmulty/layer[lay].pdivy;

    // make my life easier; don't allow scrolling past map edges
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    // get subtile position while we still have pixel precision
    x_sub = -(x & 15);
    y_sub = -(y & 15);
    // determine upper left tile coords of camera
    x >>= 4;
    y >>= 4;

    // calculate tiled rows and columns
    clip_width = (gfx.scrx + 31)/16;
    clip_length = (gfx.scry + 31)/16;

    // safeguard; this should never happen due to camera bounding
    // ie. if a map is set as visible, there should be something to draw at all times
    if (x + clip_width - 1 < 0	|| x >= layer[lay].sizex
        ||	y + clip_length - 1 < 0	|| y >= layer[lay].sizey)
    {
        return;
    }

    // clip upper left
    if (x + clip_width - 1 >= layer[lay].sizex)
        clip_width = layer[lay].sizex - x;
    if (y + clip_length - 1 >= layer[lay].sizey)
        clip_length = layer[lay].sizey - y;

    // clip lower right
    if (x < 0)
    {
        clip_width += x;
        x = 0;
    }
    if (y < 0)
    {
        clip_length += y;
        y = 0;
    }

    source = layers[lay] + y*layer[lay].sizex + x;
    y = y_sub;
    do
    {
        y_sub = x_sub; // don't try this at home
        x = clip_width;
        do
        {
            // validate tile request
            c = *source;
            if (c < 0 || c >= vsp->NumTiles())
                c = 0;
            // validate it again
            c = tileanim.GetTileIdx(c);
            if (c >= 0 && c < vsp->NumTiles())
            {
                DrawTile(y_sub, y, c);
            }
            source += 1;
            x -= 1;
            y_sub += 16;	// x screen position
        }
        while (x);
        source += (layer[lay].sizex - clip_width);
        y += 16;
        clip_length -= 1;
    }
    while (clip_length);
}*/

template <class T>
void RenderLayer(MapFile::TileLayer& l, T DrawTile)
{
    const int xmax = l.Width()*16-gfx.XRes();
    const int ymax = l.Height()*16-gfx.YRes();

    int xw=(int)(xwin*l.parx);
    int yw=(int)(ywin*l.pary);

    if (xw>=xmax) xw = xmax-1;
    if (yw>=ymax) yw = ymax-1;
    if (xw<0) xw = 0;
    if (yw<0) yw = 0;

    // the tile to be drawn in the upper left corner
    int xtile = xw/16;
    int ytile = yw/16;

    // Subtile adjustment for each tile
    int xadj=-(xw&15);
    int yadj=-(yw&15);

    // Number of tiles to blit (on each axis)
    int xlen=(gfx.XRes()+16)/16;	// round up
    int ylen=(gfx.YRes()+16)/16;

    // Clipping
    if (xtile+xlen>l.Width())   xlen = l.Width() -xtile;
    if (ytile+ylen>l.Height())  ylen = l.Height()-ytile;

    // current drawing location
    int curx = xadj;
    int cury = yadj;

    MapFile::TileLayer::Iterator iter(l);

    for (int y = 0; y<ylen; y++)
    {
        iter.MoveTo(xtile, y+ytile);

        for (int x = xlen; x; x--)
        {
            u32 t=*iter;

            // animation
            t = tileanim.GetTileIdx(t);

            if (t>=0 && t<vsp->NumTiles())
                DrawTile(curx, cury, t);

            iter++;
            curx+=16;
        }

        curx = xadj;
        cury+=16;
    }
}

namespace
{
    // Decorator pattern
    template <class _Iter, typename T>
    class AnimIter
    {
        _Iter iter;
    public:
        AnimIter(_Iter i) : iter(i){}

        inline AnimIter& operator ++(int)
        {
            iter++;
        }

        inline T& operator *()
        {
            T t=*iter;
            return tileanim.GetTileIdx(t);
        }

        void MoveTo(int x, int y)
        {
            iter.MoveTo(x, y);
        }
    };

    class GetObs
    {
    public:
        inline int operator ()(int x, int y)
        {
            return map->Obs().Get(x, y);
        }
    };

    class GetZone
    {
    public:
        inline int operator ()(int x, int y)
        {
            return map->Zone().Get(x, y);
        }
    };

    class OpaqueBlit
    {
    public:
        inline void operator ()(int x, int y, int t)
        {
            gfx.CopySprite(x, y, 16, 16, (u8*)vsp->GetTile(t));
        }
    };

    class TransparentBlit
    {
    public:
        inline void operator ()(int x, int y, int t)
        {
            if (t)
                gfx.TCopySprite(x, y, 16, 16, (u8*)vsp->GetTile(t));
        }
    };

    class LucentBlit
    {
        int mode;
    public:
        LucentBlit(int m) : mode(m) {}

        inline void operator ()(int x, int y, int t)
        {
            if (t)
                gfx.TCopySpriteLucent(x, y, 16, 16, (u8*)vsp->GetTile(t), mode);
        }
    };

    class StippleBlit
    {
    public:
        inline void operator ()(int x, int y, int t)
        {
            if (t)
                gfx.BlitStipple(x, y, t);
        }
    };
};

void BlitLayer(int lay, bool trans)
{
    if (lay < 0 || lay >= map->NumLayers())
        return;

    // hline takes precedence
    /*	if (layer[lay].hline)
    {
    ExecuteEvent(layer[lay].hline);
    }
    // solid / mask / color_mapped are backseat
    else*/
    {
        MapFile::TileLayer& layer = map->GetLayer(lay);

        if (!trans)
            RenderLayer(layer, OpaqueBlit());
        else if (layer.trans != 0)
            RenderLayer(layer, LucentBlit(layer.trans));
        else
            RenderLayer(layer, TransparentBlit());
    }
}

void DrawObstructions()
{
    RenderLayer(map->Obs(), MapFile::Layer<bool>::Iterator(map->Obs()), StippleBlit());
    /*	// the tile to be drawn in the upper left corner
    int xtile = xwin/16;
    int ytile = ywin/16;

    // Subtile adjustment for each tile
    int xadj=-(xwin&15);
    int yadj=-(ywin&15);

    // Number of tiles to blit (on each axis)
    int xlen=(gfx.XRes()+16)/16;	// round up
    int ylen=(gfx.YRes()+16)/16;

    // current drawing location
    int curx = xadj;
    int cury = yadj;

    MapFile::Layer<bool>::Iterator iter(map->Obs(), xtile, ytile);

    for (int y = 0; y<ylen; y++)
    {
    for (int x = xlen; x; x--)
    {
    bool t=*iter;

    if (t!=0)
    gfx.BlitStipple(curx, cury, 0);

    iter++;
    curx+=16;
    }

    curx = xadj;
    cury+=16;
    iter.MoveTo(xtile, y+ytile);
    }*/
}

void DrawZones()
{
    /*	int		x;
    int		y;
    int		x_sub;
    int		y_sub;
    int		temp;
    int		clip_width;
    int		clip_length;
    unsigned char*	source;

    // debugging for now
    //	if (gfx.bpp>1) return;

    // adjust view according to parallax (NO NO PARALLAX NOT HERE)
    x = xwin;
    y = ywin;

    // make my life easier; don't allow scrolling past map edges
    if (x < 0)
    x = 0;
    if (y < 0)
    y = 0;

    // get subtile position while we still have pixel precision
    x_sub = -(x & 15);
    y_sub = -(y & 15);
    // determine upper left tile coords of camera
    x >>= 4;
    y >>= 4;

    // calculate tiled rows and columns
    clip_width = (gfx.XRes() + 31)/16;
    clip_length = (gfx.YRes() + 31)/16;

    // safeguard; this should never happen due to camera bounding
    // ie. if a map is set as visible, there should be something to draw at all times
    if (x + clip_width - 1 < 0	|| x >= map->Width()
    ||	y + clip_length - 1 < 0	|| y >= map->Height())
    {
    return;
    }

    // clip upper left
    if (x + clip_width - 1 >= map->Width())
    clip_width = map->Width() - x;
    if (y + clip_length - 1 >= map->Height())
    clip_length = map->Height() - y;

    // clip lower right
    if (x < 0)
    {
    clip_width += x;
    x = 0;
    }
    if (y < 0)
    {
    clip_length += y;
    y = 0;
    }

    source = zone + y*map->Width() + x;
    y = y_sub;
    do
    {
    y_sub = x_sub; // don't try this at home
    x = clip_width;

    do
    {
    if (*source)
    gfx.BlitStipple(y_sub, y, *source);

    source += 1;
    x -= 1;
    y_sub += 16;	// x screen position
    }
    while (x);

    source += (map->Width() - clip_width);
    y += 16;
    clip_length -= 1;
    }
    while (clip_length);

    for (temp = 0; temp < entities; temp++)
    {
    x = ents[temp].x - xwin;
    y = ents[temp].y - ywin;
    gfx.BlitStipple(x, y, 32);
    }*/
}

void HookScriptThing(unsigned int &rpos)
{
    int mark = rpos+1;
    while (
        rpos < map->rstring.length() &&
        ('X' != map->rstring[rpos] && 'x' != map->rstring[rpos])
        )
    {
        rpos++;
    }

    int ev = atoi(map->rstring.substr(mark, rpos - mark).c_str());
    ExecuteEvent(ev);
}


void CameraFocusOn(Entity* focus)
{
    if (!focus)
        return;

    xwin = focus->x - gfx.scrx/2;
    ywin = focus->y - gfx.scry/2;

    // in case the map is smaller than the screen
    const int maxx = map->Width()*16 - gfx.scrx;
    const int maxy = map->Height()*16 - gfx.scry;
    if (xwin>maxx)  xwin = maxx;
    if (ywin>maxy)  ywin = maxy;
    if (xwin<0)     xwin = 0;
    if (ywin<0)     ywin = 0;
}

void DoCameraTracking()
{
    Entity*	focus;

    // there's 3 basic camera tracking modes:
    //		#1	focus on player
    //		#2	focus on specific entity; could be anyone
    //		#?	anything else disables camera tracking

    switch (cameratracking)
    {
    case 1:
        if (playeridx != -1)
            focus=&ents[playeridx];
        else
            return;
        break;
    case 2:
        if (tracker != -1)
            focus = &ents[tracker];
        else
            return;
        break;
    default:
        return;
    }

    CameraFocusOn(focus);
}

int rnd(int lo, int hi)
{
    hi = hi - lo + 1;

    if (hi > 0)
        hi = rand() % hi;
    else
        hi = 0;

    return lo + hi;
}

// ----------------------------- Entities

void DrawEntity(int i)
{
    Entity& ent = ents[i];

    if (!ent.visible || !ent.on)
        return;

    int dx = ent.x-xwin;
    int dy = ent.y-ywin;

    if (ent.chrindex < 0 || ent.chrindex >= numchrs) return;

    Sprite& sprite=*chr[ent.chrindex];

    int frame = ent.specframe ? ent.specframe : ent.curframe;

    if (frame < 0 || frame >= sprite.NumFrames())
    {
        Log::Write(va("DrawEntity: invalid frame request: %d (%d total)", frame, sprite.NumFrames()));
        frame = 0;
    }

    gfx.TCopySprite(
        dx - sprite.HotX(),
        dy - sprite.HotY(),
        sprite.Width(),
        sprite.Height(),
        (u8*)sprite.GetFrame(frame));
}

// if the entity is onscreen, then its index is added to the entidx vector.  numentsonscreen is the number of entities onscreen.
void SiftEntities()
{
    // entidx has as many elements as there are entities so that this thing isn't resized every frame. ;P
    if (entidx.size()!=entities)
        entidx.resize(entities);

    numentsonscreen = 0;

    for (int n = 0; n<entities; n++)
    {
        Entity& ent = ents[n];

        int idx = ents[n].chrindex;

        // These are the easiest to check for, so we do them first.
        if (!chr[idx])							continue;
        if (!ents[n].visible || !ents[n].on)	continue;

        int dx = ents[n].x-xwin+16;
        int dy = ents[n].y-ywin+16;

        if (dx<0 || dx>gfx.scrx+chr[ents[n].chrindex]->Width())
            continue;
        if (dy<0 || dy>gfx.scry+chr[ents[n].chrindex]->Height())
            continue;

        entidx[numentsonscreen++]=n;
    }
}

namespace
{
    class CompareEntities
    {
    public:
        inline int operator()(int a, int b)
        {
            return ents[a].y<ents[b].y;
        }
    };
};

void RenderEntities()
{
    SiftEntities();

    // qsort sucks. <:D
    std::sort(entidx.begin(), entidx.begin()+numentsonscreen, CompareEntities());

    int n = 0;
    while (n<numentsonscreen)
    {
        DrawEntity(entidx[n]);
        n++;
    }
}

// ------------------------- Main interface ------------------

void RenderMAP()
{
    static int inside = 0;
    unsigned int rpos = 0;

    tileanim.Update();                  // not ideal.  Animations will stop when VC is running.

    if (!map)   return;

    DoCameraTracking();

    int layCount = 0;

    while (rpos < map->rstring.length())
    {
        char c = map->rstring[rpos];

        if (c>='1' && c<='6')
        {
            BlitLayer(c-'1', (layCount++) != 0);
        }
        else
            switch (c)
        {
            case 'e':
            case 'E': RenderEntities(); break;
            case 's':
            case 'S': HookScriptThing(rpos); break;
            case 'r':
            case 'R': if (!inside)
                      {
                          inside = 1;
                          HookRetrace();
                          inside = 0;
                      }
                      break;
        }

        rpos += 1;
    }
    if (showobs)
        DrawObstructions();
    if (showzone)
        DrawZones();
}

void Render()
{
    //  DoCameraTracking();

    RenderMAP();

    console.Draw();
}
