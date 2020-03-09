
#include <cassert>
#include "videofilter.h"
#include "2xsai.h"
#include "hq3x.h"

FilterNone::FilterNone() 
    : _width(0)
    , _height(0)
{}

u8* FilterNone::getBackBuffer()
{
    return _backBuffer.get();
}

void FilterNone::resize(uint x, uint y)
{
    _width = x;
    _height = y;
    _backBuffer = new u8[x * y * 2];
}

void FilterNone::flip(u8* dest, uint destPitch)
{
    int y = _height;
    u8* src = _backBuffer.get();
    while (y--)
    {
        memcpy(dest, src, _width * 2);
        src += _width * 2;
        dest += destPitch;
    }
}

Filter8bpp::Filter8bpp() 
    : _width(0)
    , _height(0)
{}

u8* Filter8bpp::getBackBuffer()
{
    return _backBuffer.get();
}

void Filter8bpp::resize(uint x, uint y)
{
    _width = x;
    _height = y;
    _backBuffer = new u8[x * y];
}

void Filter8bpp::flip(u8* dest, uint destPitch)
{
    int y = _height;
    u8* src = _backBuffer.get();
    while (y--)
    {
        memcpy(dest, src, _width);
        src += _width;
        dest += destPitch;
    }
}

#ifdef VERGE_VIDEOFILTERS_ENABLED

FilterKreed::FilterKreed()
    : _width(0)
    , _height(0)
{
    const int pixelFormat = 16;
    assert(pixelFormat == 15 || pixelFormat == 16);

    Init_2xSaI(pixelFormat == 15 ? 555 : 565);
}

u8* FilterKreed::getBackBuffer()
{
    return _backBuffer.get() + (_width + 1) * 2;
}

void FilterKreed::resize(uint x, uint y)
{
    _width = x;
    _height = y;
    _backBuffer = new u8[(x + 2) * (y + 2) * 2];
    _deltaPtr   = new u8[(x * 2 + 2) * (y * 2 + 2) * 2];
}

void Filter2xSaI::flip(u8* dest, uint destPitch)
{
    ::_2xSaI(
        getBackBuffer(), _width * 2,
        _deltaPtr.get(),
        dest, destPitch,
        _width, _height);
}

void FilterEagle::flip(u8* dest, uint destPitch)
{
    ::SuperEagle(
        getBackBuffer(), _width * 2,
        _deltaPtr.get(),
        dest, destPitch,
        _width, _height);
}

FilterHqNx::FilterHqNx(uint scale)
    : _scale(scale)
{
    InitLUTs();
}

u8* FilterHqNx::getBackBuffer()
{
    return _backBuffer.get();
}

void FilterHqNx::resize(uint x, uint y)
{
    _width = x;
    _height = y;
    _backBuffer = new u8[x * y * 2];
}

FilterHq2x::FilterHq2x()
    : FilterHqNx(2)
{}

void FilterHq2x::flip(u8* dest, uint destPitch)
{
    hq2x_32(_backBuffer.get(), dest, _width, _height, destPitch);
}

FilterHq3x::FilterHq3x()
    : FilterHqNx(3)
{}

void FilterHq3x::flip(u8* dest, uint destPitch)
{
    hq3x_32(_backBuffer.get(), dest, _width, _height, destPitch);
}

FilterHq4x::FilterHq4x()
    : FilterHqNx(4)
{}

void FilterHq4x::flip(u8* dest, uint destPitch)
{
    hq4x_32(_backBuffer.get(), dest, _width, _height, destPitch);
}

#endif