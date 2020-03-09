
#include <algorithm>
#include "Event.h"

using namespace VergeC;

Event::Event(u8* data, int l)
{
    bytecode = new u8[l];
    len = l;
    std::copy(data, data+l, bytecode);
}

Event::~Event()
{
    delete bytecode;
}