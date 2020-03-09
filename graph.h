#ifndef GRAPH_H
#define GRAPH_H

#include "SDL.h"
#include "vtypes.h"
#include "misc.h"

struct VideoFilter;

class GrDriver
{
public:
    u8*  trueScreen;                    // pointer to real screen data
    int  xres, yres;                    // virtual resolution, not counting any video filters we may be using.
    bool fullScreen;                    // fullscreen/windowed flag
    std::string driverDesc;             // name of the driver

    // Look at all the look up tables! @_@
    ScopedArray<char> lucentlut8;       // table for 8bit lucency
    ScopedArray<u16> lucentlut16;       // hicolour lucency table
    ScopedArray<u16> morphlut;          // hicolour PaletteMorph emulation table

    ScopedPtr<VideoFilter> _filter;     // current video output filter

    // precalculated stuff for Supafast Lucency Stuff(tm)
    u32 lucentmask;
    int rpos , gpos , bpos;             // pixel format information
    int rsize, gsize, bsize;

    int vsync;                          // vertal sync flag

    bool MMX;                           // true if the host CPU has it

    SDL_Surface *mainSurface;

    void GetPixelFormat();              // sets rpos, rsize, etc...

    void DestroySurfaces();

public:
    GrDriver();
    ~GrDriver();

    u16     trans_mask;                 // transparent colour key

    int     bpp;                        // BYTES (not bits) per pixel
    u8*     screen;                     // the virual screen.  Can be changed with SetRenderDest()

    SRect   clip;                       // current clip rect. ;P

    u8      gamepal[768];               // the current palette, unmorphed
    u8      pal[768];                   // the current palette, post-PaletteMorph

    // initialization/etc...
    bool Init(int x, int y, int bpp, bool fullscreen, VideoFilter* filter = 0);     // starts the whole thing up
    bool SetMode(int x, int y, int bpp, bool fullscreen);               // changes the resolution/full-screenedness
    void ShutDown();
    void VSync(bool on); // turns vsync on/off

    // colour stuff
    unsigned int Conv8(int c);                          // 8 bit colour -> screen pixel
    unsigned int Conv8(u8* pal, int c);                 // same, but with palette stipulated
    unsigned int Conv16(int c);                         // 5:6:5 pixel -> screen pixel
    unsigned int PackPixel(int r, int g, int b);        // 8:8:8 pixel -> screen pixel
    void UnPackPixel(int c, int& r, int& g, int& b);    // screen pixel -> 8:8:8 pixel
    int  SetPalette(u8* p);                             // char[768]
    void UpdatePalette(u8* p);
    int  GetPalette(u8* p);                             // ditto
    int  InitLucentLUT(u8* data);                       // initializes the 8bit lookup table
    void CalcLucentLUT(int lucency);                    // set the lucency table up

    // accessors
    int  XRes();
    int  YRes();

    int scrx, scry;                                     // clip width/height

    bool IsFullScreen();
    std::string DriverDesc();

    void Clear();

    //   blits
private:
    // omni-purpose template blitter shiat.
    template <class BlendPolicy> void CopySprite16(int x, int y, int width, int height, u8* src);
    template <class BlendPolicy> void ScaleSprite16(int x, int y, int width, int height, int stretchwidth, int stretchheight, u8* src);
    template <class BlendPolicy> void RotScaleSprite16(int posx, int posy, int width, int height, float angle, float scale, u8* src);
    template <class BlendPolicy> void WrapSprite16(int x, int y, int width, int height, u8* src);
public:

    // opaque blits
    void  CopySprite(int x, int y, int width, int height, u8* src);
    void TCopySprite(int x, int y, int width, int height, u8* src);
    void  ScaleSprite(int x, int y, int iwidth, int iheight, int dwidth, int dheight, u8* src);
    void TScaleSprite(int x, int y, int iwidth, int iheight, int dwidth, int dheight, u8* src);
    void RotScale(int posx, int posy, int width, int height, float angle, float scale, u8* src);
    void  WrapBlit(int x, int y, int width, int height, u8* src);
    void TWrapBlit(int x, int y, int width, int height, u8* src);
    void BlitStipple(int x, int y, int colour);
    // silhouette

    inline void SetPixelLucent(u16* dest, int c, int lucentmode);
    // translucent blits
    void  CopySpriteLucent(int x, int y, int width, int height, u8* src, int lucentmode);
    void TCopySpriteLucent(int x, int y, int width, int height, u8* src, int lucentmode);
    void  ScaleSpriteLucent(int x, int y, int iwidth, int iheight, int dwidth, int dheight, u8* src, int lucent);
    void TScaleSpriteLucent(int x, int y, int iwidth, int iheight, int dwidth, int dheight, u8* src, int lucent);
    void RotScaleLucent(int posx, int posy, int width, int height, float angle, float scale, u8* src, int lucent);
    void WrapBlitLucent(int x, int y, int width, int height, u8* src, int lucent);
    void TWrapBlitLucent(int x, int y, int width, int height, u8* src, int lucent);


    // Primitives
    void SetPixel(int x, int y, int colour, int lucent);
    int  GetPixel(int x, int y);
    void HLine(int x, int y, int x2, int colour, int lucent);
    void VLine(int x, int y, int y2, int colour, int lucent);
    void Line(int x1, int y1, int x2, int y2, int colour, int lucent);
    void Rect(int x1, int y1, int x2, int y2, int colour, int lucent);
    void RectFill(int x1, int y1, int x2, int y2, int colour, int lucent);
    void Circle(int x, int y, int radius, int colour, int lucent);
    void CircleFill(int x, int y, int radius, int colour, int lucent);

    // polygon crap
    void FlatPoly(int x1, int y1, int x2, int y2, int x3, int y3, int colour);

protected:
    void tmaphline(int x1, int x2, int y, int tx1, int tx2, int ty1, int ty2, int texw, int texh, u8* image);
public:
    void TMapPoly(int x1, int y1, int x2, int y2, int x3, int y3,
        int tx1, int ty1, int tx2, int ty2, int tx3, int ty3,
        int tw, int th, u8 *img);

    void Mask(u8* src, u8* mask, int width, int height, u8* dest);
    void Silhouette(int width, int height, u8* src, u8*dest, int colour);
    void ChangeAll(int width, int height, u8* src, int srccolour, int destcolour);

    void ShowPage();

    // weird rendering magic stuff ;)
    void SetClipRect(SRect& clip);
    void RestoreRenderSettings();
    void SetRenderDest(int x, int y, u8* dest);

protected:
    u8 morph_step(int S, int D, int mix, int light);
public:
    void PaletteMorph(int r, int g, int b, int percent, int intensity);
};

#endif
