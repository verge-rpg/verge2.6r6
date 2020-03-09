
#include "MapFile.h"

using namespace Map;

MapFile::MapFile()
{
    obs = 0;
    zone = 0;
}

MapFile::~MapFile()
{
    for (unsigned int i = 0; i<lay.size(); i++)
        delete lay[i];
    delete obs;
    delete zone;
}

MapFile::ZoneInfo& MapFile::GetZoneInfo(unsigned int idx)
{
    static ZoneInfo dummy;
    if (idx>=zoneinfo.size())    return dummy;

    return zoneinfo[idx];
}

MapFile::TileLayer& MapFile::GetLayer(unsigned int i)
{
    if (i>=lay.size())   return *lay[0];	// out of bounds

    return *lay[i];
}

MapFile::TileLayer& MapFile::AddLayer()
{
    lay.push_back(new TileLayer());

    return **(lay.end()-1);
}

MapFile::TileLayer& MapFile::AddLayer(MapFile::TileLayer* l)
{
    lay.push_back(l);
    return *l;
}
