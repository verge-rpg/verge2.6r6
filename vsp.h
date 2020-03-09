/*
 * Not too bad, though I'm less than happy with the way the loader is set up.
 */

#ifndef VSP_H
#define VSP_H

#include "vtypes.h"
#include "Util/RefCount.h"
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
        uint first;
        uint last;
        uint delay;
        AnimMode mode;
    };

    class VSP : public RefCount
    {
        int  bpp;   // bytes per pixel.  1 or 2.
        uint numtiles;

        u8*  palette;
        u8*  pixeldata;
        std::vector<AnimStrand> anim;

    public:

        //--
        static VSP* LoadRaw(GrDriver& g, const char* filename); // loads the VSP as it is.
        static VSP* Load(GrDriver& g, const char* filename); // Loads the VSP and converts it to the proper bitdepth if necessary
        //--

        VSP();
        virtual ~VSP();

        void* GetTile(uint idx);
        void* GetPal();
        uint NumTiles();

        AnimStrand& GetAnim(int idx);
		inline int NumTileAnim() const { return 100; }  // purely to allow for expansion.

        // just for niceness:
        inline int Width()  const { return 16; }
        inline int Height() const { return 16; }
    };
};

#endif
