#include <cassert>

#include "sprite.h"
#include "graph.h"
#include "vfile.h"
#include "misc.h"
#include "log.h"

Sprite::Sprite(GrDriver& gfx, const char* fname)
{
    pixeldata.p8 = 0;

    Load(gfx, fname);
}

Sprite::~Sprite()
{
    Free();
}

void Sprite::Free()
{
    delete[] pixeldata.p8;
    pixeldata.p8 = 0;
}

void* Sprite::GetFrame(int idx) const
{
    assert(idx >=0 && idx < numframes);
    return (pixeldata.p8) + (idx * framex * framey * pixelsize);
}

int Sprite::IdleFrame(int dir) const
{
    assert(dir >=0 && dir < 4);
    return idle[dir];
}

const std::string& Sprite::GetAnim(int dir) const
{
    assert(dir >=0 && dir < 4);
    return anim[dir];
}

const std::string& Sprite::FileName() const
{
    return filename;
}

bool Sprite::Load(GrDriver& gfx, const char* fname)
{
    pixelsize = gfx.bpp;

    filename = fname;

    try
    {
        VFILE* file = vopen(filename.c_str());
        if (!file)
            throw va("Could not open CHR file %s.", fname);

        u8 ver = vgetc(file);

        if (ver != 2 && ver !=4)        throw va("CHR %s incorrect CHR format version %i.", fname, ver);
        if (gfx.bpp==1 && ver==4)       throw "Hicolor CHRs can't be loaded when in 8bit mode";

        framex = vgetw(file);
        framey = vgetw(file);

        hotx = vgetw(file);
        hoty = vgetw(file);
        hotw = vgetw(file);
        hoth = vgetw(file);


        if (ver==2)
        {
            numframes = vgetw(file);
            int buffersize = vgetq(file);

            u8* buffer = new u8[buffersize];
            vread(buffer, buffersize, file);

            int numpixels = framex * framey * numframes;

            pixeldata.p8 = new u8[numpixels];

            bool result = RLE::Read(pixeldata.p8, numpixels, buffer);
            if (!result)  throw va("LoadCHR: %s: bogus compressed image data", fname);

            delete[] buffer;

            if (gfx.bpp>1)
            {

                u16* _16 = new u16[numpixels];
                u8*  _8= pixeldata.p8;
                for (int n = 0; n<numpixels; n++)
                {
                    if (!_8[n])
                        _16[n] = gfx.trans_mask;
                    else
                        _16[n] = gfx.Conv8(_8[n]);   //LoToHi(c->imagedata[n]);
                }
                delete[] pixeldata.p8;
                pixeldata.p16 = _16;
            }

            idle[2]=vgetq(file);
            idle[3]=vgetq(file);
            idle[1]=vgetq(file);
            idle[0]=vgetq(file);

            for (int b = 0; b<4; b++)
            {

                int n = vgetq(file);
                if (n>99)
                    throw;

                char str[100];
                vread(str, n, file);

                switch (b)
                {
                case 0: anim[left ]=str;    break;
                case 1: anim[right]=str;    break;
                case 2: anim[up   ]=str;    break;
                case 3: anim[down ]=str;    break;
                }
            }
        }
        else // if (ver==4)
        {
            idle[left ]=vgetw(file);
            idle[right]=vgetw(file);
            idle[up   ]=vgetw(file);
            idle[down ]=vgetw(file);

            numframes = vgetw(file);

            for (int b = 0; b<4; b++)
            {

                int n = vgetq(file);
                if (n>99)
                    throw;

                char str[100];
                vread(str, n, file);
                str[n] = 0;

                switch (b)
                {
                case 0: anim[left ]=str;    break;
                case 1: anim[right]=str;    break;
                case 2: anim[up   ]=str;    break;
                case 3: anim[down ]=str;    break;
                }
            }

            int numpixels = framex * framey * numframes;

            pixeldata.p16 = new u16[numpixels];

            int buffersize = vgetq(file);
            u8* buffer = new u8[buffersize];
            vread(buffer, buffersize, file);

            bool result = RLE::Read(pixeldata.p16, numpixels, (u16*)buffer);
            if (!result) throw "Stuff has happened whilst loading up a hicolour CHR";

            delete[] buffer;

            // tweak transparency mask
            u16* p= pixeldata.p16;
            for (int n = numpixels; n; n--)
            {
                if (*p==0)
                    *p = gfx.trans_mask;
                p++;
            }
        }
        vclose(file);
    }
    catch (const char* s)
    {
        Log::Write(s);
        Free();
        return false;
    }

    return true;
}
