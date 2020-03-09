
#pragma once

#include "vtypes.h"

namespace VergeC
{
    class Event
    {
        u8* bytecode;
        int len;

    public:
        Event(u8* data, int l);
        ~Event();

        inline int Length() const { return len; }
        inline u8* Code() const { return bytecode; }
    };
};