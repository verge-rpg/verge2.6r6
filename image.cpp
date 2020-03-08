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
// ³                           Imaging module                            ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
// Mega kudos to aen for porting that GIF code.

/*

  mod log:

6       June        2002 been awhile :D
<andy>
*   Rewrite to use Corona

28      November    2000
<tSB>
*   Obligatory tweaking to better suit the new graphics driver interface.
  
8       January         2000
<aen>
*       Revamp/tinkering. Interface is: Image_LoadBuf(), Image_Width(), Image_Length()
    
23      December        1999
<aen>
*       Fixed PCX loading code. Again.

*/

#include <string.h>

#include "vtypes.h"
#include "corona.h"
#include "graph.h"
#include "a_memory.h"

extern GrDriver gfx;

using namespace corona;

namespace
{
    int image_width=0;
    int image_height=0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// INTERFACE //////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

int Image_Width()	{ return image_width;	}
int Image_Length()	{ return image_height;	}

u8* Image_LoadBuf(const char* filename)
{
    PixelFormat pf = gfx.bpp==1 ? PF_I8 : PF_R8G8B8;

    Image* i=OpenImage(filename,FF_AUTODETECT,pf);

    if (!i)
        return 0;   // :x

    image_width=i->getWidth();
    image_height=i->getHeight();

    u8* buffer=0;

    if (gfx.bpp==1) // indexed mode -- set system palette to match the image
    {
        gfx.SetPalette((u8*)i->getPalette());
        
        buffer=(u8*)valloc(image_width * image_height,"LoadImage",OID_IMAGE);
        memcpy(buffer, i->getPixels(), i->getWidth() * i->getHeight());
    }
    else
    {
        // Convert 8bpp images to 24bpp, after setting the system palette.
        if (i->getFormat()==PF_I8)
        {
            gfx.SetPalette((u8*)i->getPalette());

            i=ConvertImage(i,PF_R8G8B8);
        }

        buffer=(u8*)valloc(image_width * image_height * 2,"LoadImage",OID_IMAGE);
        u16* buf=(u16*)buffer;
        u8*  src=(u8*)i->getPixels();

        for (int i=0; i<image_width * image_height; i++)
        {
            u8 r=*src++;
            u8 g=*src++;
            u8 b=*src++;

            buf[i]=gfx.PackPixel(r,g,b);
        }
    }

    delete i;

    return buffer;
}

void WriteImage(u8* data,int width,int height,u8* palette,const char* filename)
{
    Image* img=CreateImage(width,height,PF_I8,256,PF_R8G8B8);

    memcpy(img->getPixels(),data,width*height);
    memcpy(img->getPalette(),palette,256*3);

    SaveImage(filename,FF_PNG,img);

    delete img;
}

void WriteImage(u16* data,int width,int height,const char* filename)
{
    Image* img=CreateImage(width,height,PF_R8G8B8A8);
    u32* temp=(u32*)img->getPixels();

    int r,g,b;
    for (int i=0; i<width*height; i++)
    {
        gfx.UnPackPixel(data[i],r,g,b);
        temp[i]=(255<<24) | (b<<16) | (g<<8) | r;
    }

    SaveImage(filename,FF_PNG,img);

    delete img;
}