
#ifndef MAPFILE_H
#define MAPFILE_H

#pragma warning (disable:4786)	// VC6 is retarded
#include <vector>
#include <string>
#include <cassert>
#include <assert.h>

#include "vtypes.h"
#include "entity.h"
#include "LoadMap.h"
#include "Util/RefCount.h"

namespace Map
{

    /*
    * The reason why we have two separate entity classes is very simple.
    * This class represents the map FILE.  It is completely separate from
    * the engine.  Interdependancy is neat stuff.
    */

    class MapFile : public RefCount
    {
        // LoadMap.h
        friend MapFile* Map::LoadMap(VFILE* file);
    public:
        struct Entity
        {
            int         x, y;                    // pixel-coords.  divide if you want tile coords.

            Direction   direction;

            int         chrindex;               // handle to the CHR

            bool        isobs;                  // can be obstructed
            bool        canobs;                 // can obstruct other ents
            int         speed;

            int         animscriptidx;

            int         movescriptidx;

            bool        autoface;
            bool        adjactivate;            // if true, the entity's actscript is run when the entity is adjacent to the player

            int			movecode;
            int         movescript;

            int         step;                   // wandering things
            int         delay;                  // ditto
            int         stepcount, delaycount;

            bool        visible;

            int         wanderzone;             // the zone the entity must stay on as it wanders (if applicable)
            SRect       wanderrect;             // the rect the entity must stay within as it wanders (if applicable)

            int         actscript;              // script called when the entity is activated

            std::string    description;            // arbitrary
        };
        // essentially a glorified 2D array
        template <typename T>
        class Layer
        {
            T* data;
            uint width;
            uint height;

        public:

            Layer()
            {
                width = height = 0;
                data = 0;
            }

            Layer(uint w, uint h, T* d)
            {
                width = w;
                height = h;
                data = new T[w*h];

                // Not using memcpy out of consideration for potential copy constructors or something.
                // Call me paranoid.

                // Maybe I should specialize this method for u32 and bool. (which are the only two types
                // I actually plan on using with this class.  Go figure. ;)
                for (uint i = 0; i<w*h; i++)
                    data[i]=d[i];
            }

            Layer(const Layer<T>& l)
                : width(l.width), height(l.height)
            {
                data = new T[width*height];

                // copy.  Fun.
                for (int i = 0; i<width*height; i++)
                    data[i]=l.data[i];
            }

            ~Layer()
            {
                delete[] data;
            }

            inline T Get(uint x, uint y) const
            {
                if (x<0 || y<0 || x>=width || y>=height)
                    return T(); // 0 for most numeric types, I believe.

                return data[y*width+x];
            }

            inline void Set(uint x, uint y, const T& value) // const bool&s are bigger than bools.  urk.  But this is inlined anyway, so bleh.
            {
                if (x<0 || y<0 || x>=width || y>=height)
                    return;

                data[y*width+x]=value;
            }

            void Resize(int newx, int newy)
            {
                // whichever is smaller
                int sx= newx<width  ? newx : width;
                int sy= newy<height ? newy : height;

                T* temp = new T[x*y];

                for (int y = 0; y<sy; y++)
                {
                    for (int x = 0; x<sx; x++)
                        temp[y*newx+x]=data[y*width+x];
                }

                delete[] data;
                data = temp;

                width = newx;
                height = newy;
            }

            inline int Width()  const { return width; }
            inline int Height() const { return height; }
            class Iterator
            {
                // This class is completely unsafe in release mode.
            private:
                Layer<T>& lay;

                T* curpos;
                uint offset;
#ifdef _DEBUG
                bool valid;
#endif

            public:
                Iterator(Layer<T>& l, int x = 0, int y = 0) : lay(l)
                {
                    offset = y*lay.width+x;
                    curpos = lay.data+offset;
#ifdef _DEBUG
                    valid= (offset<lay.width*lay.height) ;
#endif
                }

                T operator *()
                {
#ifdef _DEBUG
                    assert(valid);
#endif

                    return *curpos;
                }

                Iterator& operator ++(int)
                {
                    offset++;
                    curpos++;
#ifdef _DEBUG
                    valid= (offset<lay.width*lay.height) ;
#endif
                    return *this;
                }

                void MoveTo(int x, int y)
                {
                    offset = y*lay.width+x;
                    curpos = lay.data+offset;
#ifdef _DEBUG
                    valid= (offset<lay.width*lay.height) ;
#endif
                }
            };
        };

        class TileLayer : public Layer<u32>
        {
            typedef Layer<u32> Layeru32;	// hacking around compilers makes baby jesus cry
        public:
            float parx;
            float pary;
            int   trans;

            TileLayer() : Layeru32(), parx(0), pary(0), trans(0)
            {}

            TileLayer(int x, int y, u32* d)
                : Layeru32(x, y, d) , parx(0), pary(0), trans(0)
            {}
        };

        struct ZoneInfo
        {
            int script;
            int chance;
            bool adjactivate;
            std::string description;

            // int delay, entityscript;	// never implemented anyway
        };

        //----------------------------

    protected:
        //	public:
        /*
        * The main reason I'm hiding these is because they're pointers.
        * I don't want dumb people assigning these to anything.
        */
        Layer<bool>* obs;
        Layer<u8>* zone;

        std::vector<TileLayer*> lay;

        // oh wah.  Improper textbook OOP.
        // Cry me a river. ;)
    public:
        std::vector<Entity> entities;
        std::vector<ZoneInfo> zoneinfo;
        std::vector<std::string> chrs;
        std::vector<MoveScript> movescripts;

        std::string vspname;
        std::string musname;
        std::string rstring;

        MapFile();

        ~MapFile();

        inline int Width() const  { return lay[0]->Width();  }
        inline int Height() const { return lay[0]->Height(); }

        inline Layer<bool>& Obs() const { return *obs;   }
        inline Layer<u8>&   Zone()const { return *zone;  }

        ZoneInfo&  GetZoneInfo(uint idx);

        TileLayer& GetLayer(uint i);
        TileLayer& AddLayer();
        TileLayer& AddLayer(TileLayer* l);

        inline int NumLayers() const { return lay.size();	}
    };

};

#endif
