
#include <cassert>
#include <vector>
#include "vfile.h"
#include "MapFile.h"
#include "LoadMap.h"
#include "misc.h"
#include "log.h"

using std::vector;
using namespace Map;

namespace
{
    const char* mapsig="MAPù5";

    template <class T, typename U>
        static T* ReadLayer(int width, int height, VFILE* f)
    {
        u32 bufsize = vgetq(f);
        U* buffer = new U[bufsize];
        U* data = new U[width*height];

        vread(buffer, bufsize, f);

        RLE::Read(data, width*height, buffer);

        T* lay = new T(width, height, data);

        delete[] buffer;
        delete[] data;

        return lay;
    }


    VFILE* operator >> (VFILE* f, MapFile::Entity& e)
    {
        e.x = vgetq(f)*16;
        e.y = vgetq(f)*16;
        vgetw(f);   vgetw(f);   // tile x/y are redundant

        vgetc(f);   // face direction and move direction are one and the same
        e.direction=(Direction)vgetc(f);
        vgetc(f);//e.movecount = vgetc(f);
        vgetc(f);//e.curframe = vgetc(f);
        vgetc(f);//e.specframe = vgetc(f);
        e.chrindex = vgetc(f);
        // reset=
        vgetc(f); // ???

        e.isobs  = vgetc(f) != 0;
        e.canobs = vgetc(f) != 0;

        e.speed = vgetc(f);
        vgetc(f);//e.speedcount = vgetc(f);

        vgetc(f); // animation-frame delay (???)
        e.animscriptidx= vgetq(f);
        e.movescriptidx= vgetq(f);
        e.autoface     = vgetc(f) != 0;
        e.adjactivate  = vgetc(f) != 0;
        e.movecode     = (Entity::MovePattern)vgetc(f);
        e.movescript   = vgetc(f);
        vgetc(f); // subtile move count thing
        vgetc(f); // internal mode thing

        e.step = vgetw(f);
        e.delay = vgetw(f);
        e.stepcount = vgetw(f);
        e.delaycount = vgetw(f);

        vgetw(f); // data1
        e.wanderzone = e.wanderrect.left = vgetw(f);   // data2
        e.wanderrect.top   = vgetw(f);
        vgetw(f); // data4
        e.wanderrect.right  = vgetw(f);
        e.wanderrect.bottom= vgetw(f);
        vgetw(f); // ???
        e.actscript = vgetw(f);

        e.visible = true;

        char c[20];
        vread(c, 18, f);  // ??? skip

        vread(c, 20, f);
        c[19]=0;
        e.description = c;

        return f;
    }
};

// what a mess.
MapFile* Map::LoadMap(VFILE* file)
{
    MapFile* m = new MapFile;

    char c[255];        // temporary string buffer for a bunch of things.
    try
    {
        vread(c, 6, file);
        if (strcmp(c, mapsig))
            throw "Bogus map file.";

        vseek(file, 4, SEEK_CUR);    // skip a few bytes

        vread(c, 60, file);   m->vspname = c;
        vread(c, 60, file);   m->musname = c;
        vread(c, 20, file);   m->rstring = c;

        vseek(file, 55, SEEK_CUR);    // skip a few more

        int numlayers = vgetc(file);
        int width, height;

        vector<float> parx(numlayers);
        vector<float> pary(numlayers);
        vector<char>  trans(numlayers);

        // Layer information
        for (int i = 0; i<numlayers; i++)
        {
            int mx, my, dx, dy;
            mx = vgetc(file);
            dx = vgetc(file);
            my = vgetc(file);
            dy = vgetc(file);
            parx[i]=1.0f* mx/dx;
            pary[i]=1.0f* my/dy;

            width = vgetw(file);
            height = vgetw(file);

            trans[i]= vgetc(file);
            vgetc(file);        // hline.

            vgetw(file);        // skip two bytes
        }

        // Actual tile data
        for (int j = 0; j<numlayers; j++)
        {
            MapFile::TileLayer* l = ReadLayer<MapFile::TileLayer, u32>(width, height, file);
            l->parx = parx[j];
            l->pary = pary[j];
            l->trans = trans[j];

            m->AddLayer(l);
        }

        // Obstructions
        m->obs = ReadLayer<MapFile::Layer<bool>, bool>(width, height, file);
        // Zone placement
        m->zone = ReadLayer<MapFile::Layer<u8>, u8>(width, height, file);

        // Zone information
        u32 numzones = vgetq(file);
        m->zoneinfo.reserve(numzones);
        MapFile::ZoneInfo z;
        for (u32 i = 0; i<numzones; i++)
        {
            char name[40];
            vread(name, 40, file);
            name[39]=0;
            z.description = name;

            z.script = vgetw(file);
            z.chance = vgetw(file);
            vgetw(file);    // delay
            z.adjactivate = vgetw(file)!=0;
            vgetw(file);    // entityscript

            m->zoneinfo.push_back(z);
        }

        // CHR list
        u8 numchrs = vgetc(file);

        m->chrs.reserve(numchrs);
        for (u8 i = 0; i<numchrs; i++)
        {
            char c[61];
            vread(c, 60, file);
            c[60]=0;
            m->chrs.push_back(c);
        }

        // entities
        u8 entcount = vgetc(file);
        m->entities.resize(entcount);
        for (u8 i = 0; i<entcount; i++)
            file >> m->entities[i];

        /*
        * Move scripts
        * This is kinda wacky, as the v2 file format stores
        * a bunch of offsets to a single block of movescript
        * data.  So, we read it exactly how v2 did, then
        * reorganize it.
        */
        u8 nms = vgetc(file);
        int msbuf[100];

        assert(nms<=100);

        u32 size = vgetq(file);
        vread(msbuf, nms*4, file);
        if (nms)
        {
            char* ms = new char[size+1];
            vread(ms, size, file);
            ms[size]=0;

            m->movescripts.reserve(nms);
            for (int i = 0; i<nms; i++)
                m->movescripts.push_back(MoveScript(ms+msbuf[i]));

            delete[] ms;
        }
        else
            vseek(file, size, 0); // ???

        vgetq(file);    // number of "things" (never was implemented)
    }
    catch (const char* s)
    {
        Log::Write(va("LoadMap: %s", s));

        delete m;
        return 0;
    }

    return m;
}
