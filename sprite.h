
#ifndef SPRITE_H
#define SPRITE_H

#include <string>
#include "vtypes.h"

enum Direction
{
    down,
    up,
    left,
    right
};

class GrDriver; // graph.h

class Sprite
{
    union
    {
        u8* p8;
        u16* p16;
    } pixeldata;

    int pixelsize;          // 1 or 2
    int numframes;
    int framex, framey;

    int hotx, hoty;
    int hotw, hoth;

    int idle[4];

    std::string anim[4];

    std::string filename;

    void Free();

public:
    Sprite(GrDriver& gfx, const char* fname);
    ~Sprite();

    void* GetFrame(int idx) const;
    int IdleFrame(int dir) const;
    const std::string& GetAnim(int dir) const;
    const std::string& FileName() const;

    int NumFrames() const { return numframes; }
    int Width()  const { return framex; }
    int Height() const { return framey; }

    int HotX() const { return hotx; }
    int HotY() const { return hoty; }
    int HotH() const { return hoth; }
    int HotW() const { return hotw; }

    bool Load(GrDriver& gfx, const char* filename);
};

#endif
