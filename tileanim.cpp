#include "tileanim.h"
#include "vsp.h"

void TileAnim::Reset(VSP::VSP* vsp)
{
    pVsp = vsp;

    strand.clear();
    strand.resize(vsp->NumTileAnim());
    for (u32 i = 0; i<strand.size(); i++)
    {
        strand[i].flip = false;
        strand[i].count = vsp->GetAnim(i).delay;
    }

    tileidx.resize(vsp->NumTiles());
    for (u32 i = 0; i<tileidx.size(); i++)
        tileidx[i]=i;
}

namespace
{
    void Forward(int& cur, int first, int last)
    {
        cur++;
        if (cur>last)	cur = first;
    }

    void Backward(int& cur, int first, int last)
    {
        cur--;
        if (cur<first)	cur = last;
    }

    void Flip(int& cur, bool& flip, int first, int last)
    {
        if (flip)
        {
            cur--;
            if (cur<first)
            {
                flip=!flip;
                cur = first;
            }
        }
        else
        {
            cur++;
            if (cur>last)
            {
                flip=!flip;
                cur = last;
            }
        }
    }

    void Random(int& cur, int first, int last)
    {
        cur = rand()%(last-first) + first;
    }
};

void TileAnim::Update()
{
    for (u32 i = 0; i<strand.size(); i++)
    {
        strand[i].count--;
        if (strand[i].count > 0)
            continue;

        const VSP::AnimStrand& a = pVsp->GetAnim(i);

        strand[i].count = a.delay;

        switch (a.mode)
        {
        case VSP::forward:
            for (u32 j = a.first; j<a.last; j++)
                Forward(tileidx[j], a.first, a.last);
            break;

        case VSP::backward:
            for (u32 j = a.first; j<a.last; j++)
                Backward(tileidx[j], a.first, a.last);
            break;

        case VSP::flip:
            for (u32 j = a.first; j<a.last; j++)
                Flip(tileidx[j], strand[i].flip, a.first, a.last);
            break;

        case VSP::random:
            for (u32 j = a.first; j<a.last; j++)
                Random(tileidx[j], a.first, a.last);
            break;
        }
    }
}
