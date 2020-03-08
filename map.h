
#pragma once

#pragma warning (disable:4786)
#include <vector>

#include "vtypes.h"

class Map
{
public:
    // essentially a glorified 2D array
    template <typename T>
    class Layer
    {
        T* data;
        int width;
        int height;

    public:
        Layer(int w,int h,T* d)
        {
            width=w;
            height=h;
            data=new T[w*h];

            // Not using memcpy out of consideration for potential copy constructors or something.
            // Call me paranoid.

            // Maybe I should specialize this method for u32 and bool. (which are the only two types
            // I actually plan on using with this class.  Go figure. ;)
            for (int i=0; i<w*h; i++)
                data[i]=d[i];
        }

        ~Layer()
        {
            delete[] data;
        }

        inline T Get(int x,int y) const
        {
            if (x<0 || y<0 || x>=width || y>=height)
                return T(); // 0 for most numeric types, I believe.

            return data[y*width+x];
        }

        inline void Set(int x,int y,const T& value) // const bool&s are bigger than bools.  urk.  But this is inlined anyway, so bleh.
        {
            if (x<0 || y<0 || x>=width || y>=height)
                return;

            data[y*width+x]=value;
        }
    };

    class TileLayer : public Layer<u32>
    {
    public:
        int pmulx;
        int pdivx;
        int pmuly;
        int pdivy;
    };

    //----------------------------

private:
    Layer<bool> obs;
    Layer<u8> zone;

    std::vector<TileLayer> lay;

public:

    Layer<bool>& Obs()  { return obs;   }
    Layer<u8>&   Zone() { return zone;  }
    TileLayer&  Layer(int i)
    {
        if (i<0 || i>=lay.size())   return Layer<u32>();    // return a dummy if it's out of bounds

        return lay[i];
    }


};