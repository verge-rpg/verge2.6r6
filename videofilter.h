#ifndef VIDEOFILTER_H
#define VIDEOFILTER_H

/*
 * This is a pretty sucky design, and I know it.  oh well.
 *
 * Maybe I'll improve upon it later or something.
 *
 * Right now, all of these filters only work in 16bpp mode.
 * 8bpp versions can be written later. (they need to convert
 * bit depths on the fly.  gak)
 */

#include "vtypes.h"
#include "misc.h"

struct VideoFilter
{
    // returns the scale of the resultant image.  Should always be greater than 0
    virtual uint scale() = 0;

    // returns the bitdepth that the filter outputs.  Should be 15, 16 or 32
    virtual uint depth() = 0;

    // returns the back buffer
    virtual u8* getBackBuffer() = 0;

    // Resizes the backbuffer
    virtual void resize(uint x, uint y) = 0;

    // Does it.
    virtual void flip(u8* dest, uint destPitch) = 0;
};

struct FilterNone : VideoFilter
{
    FilterNone();

    virtual uint scale() { return 1;  }
    virtual uint depth() { return 16; }
    virtual u8* getBackBuffer();
    virtual void resize(uint x, uint y);
    virtual void flip(u8* dest, uint destPitch);

private:
    uint _width;
    uint _height;
    ScopedArray<u8> _backBuffer;
};

// standin filter for 8bpp mode.  Effectively the same as FilterNone
struct Filter8bpp : FilterNone
{
    Filter8bpp();

    virtual uint scale() { return 1;  }
    virtual uint depth() { return 8; }
    virtual u8* getBackBuffer();
    virtual void resize(uint x, uint y);
    virtual void flip(u8* dest, uint destPitch);

private:
    uint _width;
    uint _height;
    ScopedArray<u8> _backBuffer;
};

// abstract superclass for kreed's filters, since they behave nearly identically.
struct FilterKreed : VideoFilter
{
    FilterKreed();

    virtual uint scale() { return 2;  }
    virtual uint depth() { return 16; }
    virtual void resize(uint x, uint y);
    virtual u8* getBackBuffer();
    // flip is unimplemented.  This class is abstract.

protected:
    uint _width;
    uint _height;
    ScopedArray<u8> _deltaPtr;

    ScopedArray<u8> _backBuffer;
};

struct Filter2xSaI : FilterKreed
{
    virtual void flip(u8* dest, uint destPitch);
};

struct FilterEagle : FilterKreed
{
    virtual void flip(u8* dest, uint destPitch);
};

// The hqNx filters are all similar too
struct FilterHqNx : VideoFilter
{
    FilterHqNx(uint scale);

    virtual uint scale() { return _scale; }
    virtual uint depth() { return 32; }
    virtual u8* getBackBuffer();
    virtual void resize(uint x, uint y);
    // no flip.  This class is abstract.

protected:
    const int _scale;
    uint _width;
    uint _height;
    ScopedArray<u8> _backBuffer;
};

struct FilterHq2x : FilterHqNx
{
    FilterHq2x();
    virtual void flip(u8* dest, uint destPitch);
};

struct FilterHq3x : FilterHqNx
{
    FilterHq3x();
    virtual void flip(u8* dest, uint destPitch);
};

struct FilterHq4x : FilterHqNx
{
    FilterHq4x();
    virtual void flip(u8* dest, uint destPitch);
};

#endif