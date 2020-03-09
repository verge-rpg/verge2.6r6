#include <cassert>
#include "Util/RefPtr.h"
#include "vsp.h"
#include "vfile.h"
#include "misc.h"
#include "graph.h"
#include "log.h"

namespace VSP
{
    inline void Convert8bppTo16bpp(GrDriver& gfx, u8* src, u8* pal, u16* dest, int numpixels)
    {
        for (int i = 0; i<numpixels; i++)
        {
            char c = i&255;
            *dest++=gfx.Conv8(pal, *src++);
        }
    }

    VSP* VSP::LoadRaw(GrDriver& gfx, const char* fname)
    {
        VFILE*    file = 0;
    
        // Mwahaha! The Indefatigable Grue has snuck into the V2 source code! It is forever corrupted by his evil touch! Cower in fear, oh yes, FEAR! MwahahaHA..ha...hem...
        // FIXME: 8 bit palettes are getting mangled in here somewhere :P

        VSP*  vsp = new VSP;
        
        try
        {
            file =vopen(fname);
            if (!file)              throw "Unable to open file for reading";

            u16 ver = 0;
            vread(&ver, 2, file);
            if (ver>5)              throw "Invalid VSP";
            if (ver<=3) // 8bpp
            {
                vsp->bpp = 1;
                vsp->palette = new u8[3*256];
                vread(vsp->palette, 3*256, file);
                for (int i = 0; i<3*256; i++)
                    vsp->palette[i]*=4;
            }
            else
                vsp->bpp = 2;

            vread(&vsp->numtiles, 2, file);
            if (vsp->numtiles<1)    throw va("reports %i tiles!  Aborting.", vsp->numtiles);

            const int datasize = vsp->numtiles * vsp->Width() * vsp->Height();    // size, in pixels (not bytes)

            vsp->pixeldata = new u8[datasize];
    
            switch (ver)
            {
            case 2:
                vread(vsp->pixeldata, datasize, file);    // old version; raw data
                break;
            
            case 3: 
                {
                    int buffersize;                 // size of compressed data block
                    vread(&buffersize, 4, file);    // get the data size

                    u8* buffer = new u8[buffersize];  // allocate room for it
                    vread(buffer, buffersize, file);    // read the compressed data
                
                    bool result = RLE::Read(vsp->pixeldata, datasize, buffer);
                    if (!result)
                        throw "bogus compressed image data";

                    delete[] buffer;
                    break;
                }

            case 4:
                {
                    u16* d16=(u16*)vsp->pixeldata;

                    vread(vsp->pixeldata, vsp->numtiles*512, file);   // hicolor uncompressed (from v2+i) - tSB
                    
                    for (uint n = 0; n<vsp->numtiles * 256; n++)
                    {
                        u16 b = d16[n];
                        if (b)
                        {
                            u16 r=(b>>10);
                            u16 g=((b>>5)&31);
                            u16 b=(b&31);
                            // Gah! @_@
                            d16[n]=(r<<11)+(g<<6)+b; // v4 vsps are 1:5:5:5, 16bit color is 5:6:5
                        }
                        else
                            d16[n]=gfx.trans_mask;
                        
                    }
                    break;
                }
            case 5:
                {
                    u16* d16=(u16*)vsp->pixeldata;

                    int buffersize = 0;
                    vread(&buffersize, 4, file);

                    u16* buffer =new u16[buffersize / sizeof(u16)]; // buffersize in bytes

                    vread(buffer, buffersize, file);

                    bool result = RLE::Read(d16, vsp->numtiles*256, buffer);
                    if (!result) throw "Bogus compressed image data";
                     
                    delete[] buffer;
                    break;
                }

            default:
                throw va("Bogus version: %i", ver);
            }
    
            vsp->anim.resize(100);
            for (int i = 0; i<100; i++)
            {
                u16 w;
                AnimStrand& s = vsp->anim[i];

                vread(&w, 2, file);       s.first = w;
                vread(&w, 2, file);       s.last = w;
                vread(&w, 2, file);       s.delay = w;
                vread(&w, 2, file);       s.mode=(AnimMode)w;
            }
    
            vclose(file);
        }
        catch (const char* msg)
        {
            Log::Write(va("VSP8::Load: %s %s", fname, msg));
            delete vsp;
            vclose(file);
            return 0;
        }

        return vsp;
    }

    VSP* VSP::Load(GrDriver& g, const char* filename)
    {
        RefPtr<VSP> vsp = LoadRaw(g, filename);

        if (vsp->bpp==2 && g.bpp==1)    // can't use hicolour shiat in 256 colour mode
            return 0;

        if (vsp->bpp==1 && g.bpp==2)    // have to convert the 8bpp stuff to 16bpp
        {
            int s = vsp->Width()*vsp->Height()*vsp->NumTiles();
            u16* newdata = new u16[s];
            Convert8bppTo16bpp(g, vsp->pixeldata, vsp->palette, newdata, s);
            delete[] vsp->pixeldata;
            vsp->pixeldata=(u8*)newdata;
            vsp->bpp = 2;
        }

        return vsp;
    }

    //////////////////////////////////////////////////////////////////////////
    // 8bpp code                                                            //
    //////////////////////////////////////////////////////////////////////////

    VSP::VSP() : bpp(0), numtiles(0), palette(0), pixeldata(0)
    {}

    VSP::~VSP()
    {
        delete[] pixeldata;
        delete[] palette;
    }

    void* VSP::GetTile(uint idx)
    {
        assert(idx>=0 && idx<numtiles);

        return pixeldata + idx * Width() * Height() * bpp;
    }

    void* VSP::GetPal()
    {
        return palette;
    }

    AnimStrand& VSP::GetAnim(int idx)
    {
        assert(idx>=0 && idx<NumTileAnim());

        return anim[idx];
    }

    uint VSP::NumTiles()    {   return numtiles;   }

};