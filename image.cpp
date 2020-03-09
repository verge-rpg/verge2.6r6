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
// |                           Imaging module                            |
// \---------------------------------------------------------------------/
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

namespace
{
    int image_width = 0;
    int image_height = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// INTERFACE //////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

int Image_Width()	{ return image_width;	}
int Image_Length()	{ return image_height;	}

u8* Image_LoadBuf(const char* filename)
{
    corona::PixelFormat pf = gfx.bpp==1 ? corona::PF_I8 : corona::PF_R8G8B8;

    corona::Image* i = corona::OpenImage(filename, corona::FF_AUTODETECT, pf);

    if (!i)
        return 0;   // :x

    image_width = i->getWidth();
    image_height = i->getHeight();

    u8* buffer = 0;

    if (gfx.bpp == 1) // indexed mode -- set system palette to match the image
    {
        if (i->getPaletteFormat() == corona::PF_R8G8B8A8)
        {
            // blech.  dump the alpha channel
            u8 pal[768];
            u8* s = (u8*)i->getPalette();
            u8* d = &pal[0];
            for (int j = 0; j < 256; j++)
                // grab three bytes, skip one.
            {   *d++ = *s++; *d++ = *s++; *d++ = *s++; s++;  }
            gfx.SetPalette(pal);
        }
        else
            gfx.SetPalette((u8*)i->getPalette());

        buffer = (u8*)valloc(image_width * image_height, "LoadImage", OID_IMAGE);
        memcpy(buffer, i->getPixels(), i->getWidth() * i->getHeight());
    }
    else
    {
        // Convert 8bpp images to 24bpp, after setting the system palette.
        if (i->getFormat() == corona::PF_I8)
        {
            gfx.SetPalette((u8*)i->getPalette());

            i = corona::ConvertImage(i, corona::PF_R8G8B8);
        }

        buffer=(u8*)valloc(image_width * image_height * 2, "LoadImage", OID_IMAGE);
        u16* buf = (u16*)buffer;
        u8*  src = (u8*)i->getPixels();

        for (int i = 0; i<image_width * image_height; i++)
        {
            u8 r = *src++;
            u8 g = *src++;
            u8 b = *src++;

            if (r==255 && g==0 && b==255)   // transparent?
                buf[i]=gfx.trans_mask;
            else
                buf[i]=gfx.PackPixel(r, g, b);
        }
    }

    delete i;

    return buffer;
}

void WriteImage(u8* data, int width, int height, u8* palette, const char* filename)
{
    corona::Image* img = corona::CreateImage(width, height, corona::PF_I8, 256, corona::PF_R8G8B8);

    memcpy(img->getPixels(), data, width*height);
    memcpy(img->getPalette(), palette, 256);

    corona::SaveImage(filename, corona::FF_PNG, img);

    delete img;
}

void WriteImage(u16* data, int width, int height, const char* filename)
{
    corona::Image* img = corona::CreateImage(width, height, corona::PF_R8G8B8A8);
    u32* temp=(u32*)img->getPixels();

    int r, g, b;
    for (int i = 0; i<width*height; i++)
    {
        gfx.UnPackPixel(data[i], r, g, b);
        temp[i]=(255<<24) | (b<<16) | (g<<8) | r;
    }

    corona::SaveImage(filename, corona::FF_PNG, img);

    delete img;
}
