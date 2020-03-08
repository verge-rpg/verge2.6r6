
#pragma once

#include "vtypes.h"
#include "vsp.h"

struct AnimState : public VSP::AnimStrand
{
    int cur;
    int count;
    bool flip;

    void Update();
};