
#pragma once

#include "vtypes.h"
#include "strk.h"

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
    void* pixeldata;
    int pixelsize;          // 1 or 2
    int numframes;
    int framex,framey;

    int hotx,hoty;
    int hotw,hoth;

    int idle[4];

    string_k anim[4];

    string_k filename;

    void Free();

public:
    Sprite(GrDriver& gfx,const char* fname);
    ~Sprite();

    void* GetFrame(int idx) const;
    int IdleFrame(int dir) const;
    const string_k& GetAnim(int dir) const;
    const string_k& FileName() const;

    int NumFrames() const { return numframes; }
    int Width()  const { return framex; }
    int Height() const { return framey; }

    int HotX() const { return hotx; }
    int HotY() const { return hoty; }
    int HotH() const { return hoth; }
    int HotW() const { return hotw; }

    bool Load(GrDriver& gfx,const char* filename);
};