
#ifndef LOADMAP_H
#define LOADMAP_H

#include "vtypes.h"

struct VFILE;

namespace Map
{
    class MapFile;

    MapFile* LoadMap(VFILE* f);
};

#endif
