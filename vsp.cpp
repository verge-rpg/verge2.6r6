#include "vsp.h"
#include "vfile.h"
#include "misc.h"
#include "graph.h"
#include "log.h"

int ReadCompressedLayer1(u8*, int, char*); // engine.cpp
int ReadCompressedLayer2(u16*, int, u16*);

namespace VSP
{
    VSP* VSP::LoadVSP(GrDriver& g,const char* filename)
    {
        if (g.bpp==1)     // palettized mode
            return VSP8::Load(g,filename);
        else
            return VSP16::Load(g,filename);
    }

    //////////////////////////////////////////////////////////////////////////
    // 8bpp code                                                            //
    //////////////////////////////////////////////////////////////////////////

    void* VSP8::GetTile(int idx)
    {
        // ASSERT....

        return pixeldata + idx * Width() * Height();
    }

    void* VSP8::GetPal()
    {
        return palette;
    }

    AnimStrand& VSP8::GetAnim(int idx)
    {
        // ASSERT...

        return anim[idx];
    }

    int VSP8::NumTiles()    {   return numtiles;   }

    VSP8::~VSP8()
    {
        delete[] pixeldata;
        delete[] palette;
    }

    VSP8* VSP8::Load(GrDriver& gfx,const char* fname)
    {
        VFILE*    file=0;
    
        // Mwahaha! The Indefatigable Grue has snuck into the V2 source code! It is forever corrupted by his evil touch! Cower in fear, oh yes, FEAR! MwahahaHA..ha...hem...
        // FIXME: 8 bit palettes are getting mangled in here somewhere :P

        VSP8*  vsp=new VSP8;
        
        try
        {
            file =vopen(fname);
            if (!file)              throw "Unable to open file for reading";

            u16 ver=0;
            vread(&ver, 2, file);
            if (ver>5)              throw "Invalid VSP";
            if (ver>3)              throw "Unable to load hicolour VSPs in 8bit mode";
    
            // vsp's, and thus map's, palette
            vsp->palette=new u8[3*256];
            vread(vsp->palette, 3*256, file);
    
            vread(&vsp->numtiles, 2, file);
            //if (numtiles < 1)   numtiles = 1;
            if (vsp->numtiles<1)    throw va("reports %i tiles!  Aborting.",vsp->numtiles);

            const int datasize=vsp->numtiles * vsp->Width() * vsp->Height();

            vsp->pixeldata=new u8[datasize];
    
            switch (ver)
            {
            case 2:
                vread(vsp->pixeldata, datasize, file);    // old version; raw data
                break;
            
            case 3: 
                {
                    int buffersize;                 // size of compressed data block
                    vread(&buffersize, 4, file);    // get the data size

                    char* buffer=new char[buffersize];  // allocate room for it
                    vread(buffer, buffersize, file);    // read the compressed data
                
                    int result=ReadCompressedLayer1(vsp->pixeldata, datasize, buffer);    // decompress
                    if (result!=0)
                        throw "bogus compressed image data";

                    delete[] buffer;
                    break;
                }

            case 4:
            case 5:
                throw "Cannot load hicolour VSPs in 8bpp mode.";

            default:
                throw va("Bogus version: %i", ver);
            }
    
            vsp->anim.resize(100);
            for (int i=0; i<100; i++)
            {
                int w;
                AnimStrand& s=vsp->anim[i];

                vread(&w,2,file);       s.first=w;
                vread(&w,2,file);       s.last=w;
                vread(&w,2,file);       s.delay=w;
                vread(&w,2,file);       s.mode=(AnimMode)w;
            }
    
            vclose(file);
        }
        catch (const char* msg)
        {
            Log::Write(va("VSP8::Load: %s %s",fname,msg));
            delete vsp;
            vclose(file);
            return 0;
        }
        
        // allocate and build tileidx.
/*        tileidx=(unsigned short *) valloc(2*numtiles, "tileidx", OID_MISC);
        for (n=0; n<numtiles; n++)
            tileidx[n]=(unsigned short)n;
    
        // for ping-pong mode
        flipped=(char *) valloc(numtiles, "flipped", OID_MISC);
    
        animate=TRUE;*/

        return vsp;
    }

    //////////////////////////////////////////////////////////////////////////
    // 16bpp code                                                           //
    //////////////////////////////////////////////////////////////////////////
    void* VSP16::GetTile(int idx)
    {
        // ASSERT....

        return pixeldata + idx * Width() * Height();
    }

    void* VSP16::GetPal()
    {
        return palette;
    }

    AnimStrand& VSP16::GetAnim(int idx)
    {
        // ASSERT...

        return anim[idx];
    }

    int VSP16::NumTiles()    {   return numtiles;   }

    VSP16::~VSP16()
    {
        delete[] palette;
        delete[] pixeldata;
    }

    // Converts a bunch of 8bpp data to 16bpp.
    inline void Convert8bppTo16bpp(GrDriver& gfx,u8* src,u8* pal,u16* dest,int numpixels)
    {
        for (int i=0; i<numpixels; i++)
        {
            char c=i&255;
            *dest++=gfx.Conv8(pal,*src++);
        }
    }

    VSP16* VSP16::Load(GrDriver& gfx,const char* fname)
    {
        VFILE*    file=0;
    
        // Mwahaha! The Indefatigable Grue has snuck into the V2 source code! It is forever corrupted by his evil touch! Cower in fear, oh yes, FEAR! MwahahaHA..ha...hem...
        // FIXME: 8 bit palettes are getting mangled in here somewhere :P

        VSP16*  vsp=new VSP16;
        
        try
        {
            file =vopen(fname);
            if (!file)              throw "Unable to open file for reading";

            u16 ver=0;
            vread(&ver, 2, file);
            if (ver>5)              throw "Invalid VSP";
    
            // vsp's, and thus map's, palette
            if (ver<4)
            {
                vsp->palette=new u8[3*256];
                vread(vsp->palette, 3*256, file);
            }
    
            vread(&vsp->numtiles, 2, file);
            //if (numtiles < 1)   numtiles = 1;
            if (vsp->numtiles<1)    throw va("reports %i tiles!  Aborting.",vsp->numtiles);

            const int datasize=vsp->numtiles * vsp->Width() * vsp->Height();
            vsp->pixeldata=new u16[datasize];
    
            switch (ver)
            {
            case 2:
                {
                    u8* tempdata=new u8[datasize];
                    vread(tempdata, datasize, file);    // old version; raw data
                    Convert8bppTo16bpp(gfx,tempdata,vsp->palette,vsp->pixeldata,datasize);
                    delete[] tempdata;
                    
                    break;
                }
            
            case 3: 
                {
                    u8* tempdata=new u8[datasize];

                    int buffersize;                 // size of compressed data block
                    vread(&buffersize, 4, file);    // get the data size

                    char* buffer=new char[buffersize];  // allocate room for it
                    vread(buffer, buffersize, file);    // read the compressed data
                
                    int result=ReadCompressedLayer1(tempdata, datasize, buffer);    // decompress
                    if (result!=0)
                        throw "Bogus compressed image data";

                    Convert8bppTo16bpp(gfx,tempdata,vsp->palette,vsp->pixeldata,datasize);

                    delete[] tempdata;
                    delete[] buffer;
                    break;
                }

            case 4: 
                {
                    vread(vsp->pixeldata, vsp->numtiles*512, file);   // hicolor uncompressed (from v2+i) - tSB
                    
                    for (int n=0; n<vsp->numtiles * 256; n++)
                    {
                        u16 b=vsp->pixeldata[n];
                        if (b)
                        {
                            u16 r=(b>>10);
                            u16 g=((b>>5)&31);
                            u16 b=(b&31);
                            // Gah! @_@
                            vsp->pixeldata[n]=(r<<11)+(g<<6)+b; // v4 vsps are 1:5:5:5, 16bit color is 5:6:5
                        }
                        else
                            vsp->pixeldata[n]=gfx.trans_mask;
                        
                    }
                    break;
                }
            case 5: 
                {
                    int buffersize=0;
                    vread(&buffersize,4,file);

                    u8* buffer =new u8[buffersize];

                    vread(buffer,buffersize,file);

                    int result=ReadCompressedLayer2(vsp->pixeldata, vsp->numtiles*256, (u16*)buffer);
                    if (result!=0) throw "Bogus compressed image data";
                     
                    delete[] buffer;
                    break;
                }

            default:
                throw va("Bogus version: %i", ver);
            }
    
            vsp->anim.resize(100);
            for (int i=0; i<100; i++)
            {
                int w;
                AnimStrand& s=vsp->anim[i];

                vread(&w,2,file);       s.first=w;
                vread(&w,2,file);       s.last=w;
                vread(&w,2,file);       s.delay=w;
                vread(&w,2,file);       s.mode=(AnimMode)w;
            }
    
            vclose(file);
        }
        catch (const char* msg)
        {
            Log::Write(va("VSP16::Load: %s %s",fname,msg));

            delete vsp;
            vclose(file);
            return 0;
        }
        
        // allocate and build tileidx.
/*        tileidx=(unsigned short *) valloc(2*numtiles, "tileidx", OID_MISC);
        for (n=0; n<numtiles; n++)
            tileidx[n]=(unsigned short)n;
    
        // for ping-pong mode
        flipped=(char *) valloc(numtiles, "flipped", OID_MISC);
    
        animate=TRUE;*/

        return vsp;
    }

};