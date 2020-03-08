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
// ³                          Rendering module                           ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

#include "verge.h"
#include "misc.h"
#include <math.h>
#include <algorithm>

// INTERFACE DATA //////////////////////////////////////////////////////////////////////////////////

unsigned char	animate		= 0;
unsigned char	cameratracking	= 1;
unsigned char	tracker		= 0;
unsigned char	showobs		= 0;
unsigned char	showzone	= 0;

// IMPLEMENTATION DATA /////////////////////////////////////////////////////////////////////////////

static u8 curlayer = 0;

// IMPLEMENTATION CODE /////////////////////////////////////////////////////////////////////////////

// -------------------------- Map ----------------------------

template <typename T>
void Map_BlitLayer(int lay,T DrawTile)
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
			//if (c < 0 || c >= vsp->NumTiles())
			//	c = 0;
		    // validate it again
			//c = tileidx[c];
            if (c >= 0 && c < vsp->NumTiles())
            {
                DrawTile(y_sub,y,c);
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
}

namespace
{
    class OpaqueBlit
    {
    public:
        inline void operator ()(int x,int y,int t)
        {
            gfx.CopySprite(x,y,16,16,(u8*)vsp->GetTile(t));
        }
    };
    
    class TransparentBlit
    {
    public:
        inline void operator ()(int x,int y,int t)
        {
            if (t)
                gfx.TCopySprite(x,y,16,16,(u8*)vsp->GetTile(t));
        }
    };
    
    class LucentBlit
    {
        int mode;
    public:
        LucentBlit(int m) : mode(m) {}
        
        inline void operator ()(int x,int y,int t)
        {
            if (t)
                gfx.TCopySpriteLucent(x,y,16,16,(u8*)vsp->GetTile(t),mode);
        }
    };
};

void BlitLayer(int lay)
{
	if (lay < 0 || lay >= numlayers)
		return;

// hline takes precedence
	if (layer[lay].hline)
	{
		ExecuteEvent(layer[lay].hline);
	}
// solid / mask / color_mapped are backseat
	else
	{
        if (lay==0)
        {
            Map_BlitLayer(lay,OpaqueBlit());
            return;
        }
        
        if (layer[lay].trans)
        {
            Map_BlitLayer(lay,LucentBlit(layer[lay].trans));
            return;
        }

        Map_BlitLayer(lay,TransparentBlit());
	}
}

void DrawObstructions()
{
	int		x;
	int		y;
	int		x_sub;
	int		y_sub;
	int		clip_width;
	int		clip_length;
	unsigned char*	source;

// debugging for now
//	if (gfx.bpp>1) return;

// adjust view according to parallax
	x = xwin*layer[0].pmultx/layer[0].pdivx;
	y = ywin*layer[0].pmulty/layer[0].pdivy;

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
	if (x + clip_width - 1 < 0	|| x >= layer[0].sizex
	||	y + clip_length - 1 < 0	|| y >= layer[0].sizey)
	{
		return;
	}

// clip upper left
	if (x + clip_width - 1 >= layer[0].sizex)
		clip_width = layer[0].sizex - x;
	if (y + clip_length - 1 >= layer[0].sizey)
		clip_length = layer[0].sizey - y;

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

	source = obstruct + y*layer[0].sizex + x;
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

		source += (layer[0].sizex - clip_width);
		y += 16;
		clip_length -= 1;
	}
	while (clip_length);
}

void DrawZones()
{
	int		x;
	int		y;
	int		x_sub;
	int		y_sub;
	int		temp;
	int		clip_width;
	int		clip_length;
	unsigned char*	source;

// debugging for now
//	if (gfx.bpp>1) return;

// adjust view according to parallax
	x = xwin*layer[0].pmultx/layer[0].pdivx;
	y = ywin*layer[0].pmulty/layer[0].pdivy;

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
	if (x + clip_width - 1 < 0	|| x >= layer[0].sizex
	||	y + clip_length - 1 < 0	|| y >= layer[0].sizey)
	{
		return;
	}

// clip upper left
	if (x + clip_width - 1 >= layer[0].sizex)
		clip_width = layer[0].sizex - x;
	if (y + clip_length - 1 >= layer[0].sizey)
		clip_length = layer[0].sizey - y;

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

	source = zone + y*layer[0].sizex + x;
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

        source += (layer[0].sizex - clip_width);
		y += 16;
		clip_length -= 1;
	}
	while (clip_length);

	for (temp = 0; temp < entities; temp++)
	{
		x = ents[temp].x - xwin;
		y = ents[temp].y - ywin;
		gfx.BlitStipple(x, y, 32);
	}
}

void HookScriptThing(int &rpos)
{
    int mark=rpos+1;
	while (
        rpos<rstring.length() &&
	    ('X' != rstring[rpos] && 'x' != rstring[rpos])
        )
	{
		rpos++;
	}
	
    int ev=atoi(rstring.mid(mark,rpos-mark).c_str());
	ExecuteEvent(ev);
}


void CameraFocusOn(Entity* focus)
{
	if (!focus)
		return;

    xwin = focus->x - gfx.scrx/2;
    ywin = focus->y - gfx.scry/2;

    // in case the map is smaller than the screen
    const int maxx = layer[0].sizex*16 - gfx.scrx;
    const int maxy = layer[0].sizey*16 - gfx.scry;
    if (xwin>maxx)  xwin=maxx;
    if (ywin>maxy)  ywin=maxy;
    if (xwin<0)     xwin=0;
    if (ywin<0)     ywin=0;
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
    case 1: focus=&ents[playeridx]; break;
    case 2: focus=&ents[tracker];   break;
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

void AnimateTile(int i, int l)
{
/*	if (i >= 100)
		return;
	if (l < 0 || l >= numtiles)
		return;

	switch (vspanim[i].mode)
	{
	case 0:
		if (tileidx[l] < vspanim[i].finish)
			tileidx[l] += 1;
		else
			tileidx[l] = vspanim[i].start;
		break;

	case 1:
		if (tileidx[l] > vspanim[i].start)
			tileidx[l] -= 1;
		else
			tileidx[l] = vspanim[i].finish;
		break;

	case 2:
		tileidx[l] = (unsigned short) rnd(vspanim[i].start, vspanim[i].finish);
		break;

	case 3:
		if (flipped[l])
		{
			if (tileidx[l] != vspanim[i].start)
				tileidx[l] -= 1;
			else
			{
				tileidx[l] += 1;
				flipped[l] = 0;
			}
		}
		else
		{
			if (tileidx[l] != vspanim[i].finish)
				tileidx[l] += 1;
			else
			{
				tileidx[l] -= 1;
				flipped[l] = 1;
			}
		}
	}*/
}

void Animate(int i)
{
/*	int l;

	 vadelay[i] = 0;
	 for (l = vspanim[i].start; l <= vspanim[i].finish; l += 1)
	 	AnimateTile(i, l);*/
}

void CheckTileAnimation()
{
/*	int i;

	if (!animate) return;
	if (!vsp) return;

	for (i = 0; i < 100; i += 1)
	{
		if ((vspanim[i].delay) && (vspanim[i].delay < vadelay[i]))
			Animate(i);
		vadelay[i] += 1;
	}*/
}

// ----------------------------- Entities

void DrawEntity(int i)
{
    Entity& ent=ents[i];
    
    if (!ent.visible || !ent.on)
        return;

    int dx = ent.x-xwin;
    int dy = ent.y-ywin;
    
    if (ent.chrindex < 0 || ent.chrindex >= numchrs) return;
    
    Sprite& sprite=*chr[ent.chrindex];

    int frame = ent.specframe ? ent.specframe : ent.curframe;
    
    if (frame < 0 || frame >= sprite.NumFrames())
    {
        Log::Write(va("DrawEntity: invalid frame request: %d (%d total)", frame,sprite.NumFrames()));
        frame=0;
    }
    
    gfx.TCopySprite(
        dx - sprite.HotX(),
        dy - sprite.HotY(),
        sprite.Width(),
        sprite.Height(),
        (u8*)sprite.GetFrame(frame));
}

// if the entity is onscreen, then its index is added to the entidx vector.  cc is the number of entities onscreen, apparently.
void SiftEntities()
{
    // entidx has as many elements as there are entities so that this thing isn't resized every frame. ;P
    if (entidx.size()!=entities)
        entidx.resize(entities);

    cc=0;

    for (int n=0; n<entities; n++)
    {
        Entity& ent=ents[n];

        int idx=ents[n].chrindex;

        if (!chr[idx])
            continue;

        int dx=ents[n].x-xwin+16;
        int dy=ents[n].y-ywin+16;
        
        if (dx<0 || dx>gfx.scrx+chr[ents[n].chrindex]->Width())
            continue;
        if (dy<0 || dy>gfx.scry+chr[ents[n].chrindex]->Height())
            continue;

        if (!ents[n].visible || !ents[n].on)
            continue;
        
        entidx[cc++]=n;
    }
}

namespace
{
    class CompareEntities
    {
    public:
        inline int operator()(int a,int b)
        {
            return ents[a].y<ents[b].y;
        }
    };
};

void RenderEntities()
{
    SiftEntities();
    
    // qsort sucks. <:D
    std::sort(entidx.begin(),entidx.begin()+cc,CompareEntities());

    int n=0;
    while (n<cc)
    {
        DrawEntity(entidx[n]);
        n++;
    }
}

// ------------------------- Main interface ------------------

void RenderMAP()
{
    static int inside=0;
    int rpos=0;
        
    curlayer = 0;
    
    while (rpos < rstring.length())
    {
        char c=rstring[rpos];

        if (c>='1' && c<='6')
        {
            BlitLayer(c-'1');
            curlayer++;
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
                          inside=1;
                          HookRetrace();
                          curlayer++;
                          inside=0;
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
    DoCameraTracking();
    
    RenderMAP();
}
