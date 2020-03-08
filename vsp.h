
#pragma once

#include "vtypes.h"
#include <vector>

class GrDriver;     // graph.h

namespace VSP
{
    enum AnimMode
    {
        forward,
        backward,
        flip,
        random
    };

    struct AnimStrand
    {
        int first;
        int last;
        int delay;
        AnimMode mode;
    };
        
    class VSP
    {
        bool valid;
    public:

        virtual void* GetTile(int idx)=0;
        virtual void* GetPal()=0;
        virtual int NumTiles()=0;

        virtual AnimStrand& GetAnim(int idx)=0;

        static VSP* LoadVSP(GrDriver& g,const char* filename);

        // just for niceness:
        inline int Width()  const { return 16; }
        inline int Height() const { return 16; }

        // so things can derive and be happy
        virtual ~VSP() {}
    };

    // A 8bpp representation of a tileset.
    class VSP8 : public VSP
    {
        u8* palette;
        u8* pixeldata;
        int numtiles;

        std::vector<AnimStrand> anim;

        VSP8() : palette(0),pixeldata(0),numtiles(0){}

    public:

        virtual void* GetTile(int idx);
        virtual void* GetPal();
        virtual int NumTiles();

        virtual AnimStrand& GetAnim(int idx);

        static VSP8* Load(GrDriver& gfx,const char* fname);

        virtual ~VSP8();
    };

    // A 16bpp representation of a tileset.
    class VSP16 : public VSP
    {
        u16* pixeldata;
        u8*  palette;
        int numtiles;

        std::vector<AnimStrand> anim;

        VSP16() : pixeldata(0),palette(0),numtiles(0){}

    public:

        virtual void* GetTile(int idx);
        virtual void* GetPal(); // returns 0 if the palette isn't a converted 256 colour pal
        virtual int NumTiles();

        virtual AnimStrand& GetAnim(int idx);

        static VSP16* Load(GrDriver& gfx,const char* fname);
        
        virtual ~VSP16();
    };

};