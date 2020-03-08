#ifndef w_graph_h
#define w_graph_h

#include "SDL.h"
#include "vtypes.h"

class GrDriver
{
protected:
    u8* pTruescreen;          // pointer to real screen data
    int xres,yres;
    bool bFullscreen;           // fullscreen/windowed flag
    char driverdesc[255];      // string carrying the name of the driver
    
    // Look at all the look up tables! @_@
    char* lucentlut8;          // table for 8bit lucency
    u16* lucentlut16;         // hicolour lucency table
    u16* morphlut;            // hicolour PaletteMorph emulation table
    
    // precalculated stuff for tSB's Supafast Lucency Stuff(tm)
    u32 lucentmask;
    int rpos, gpos ,bpos;      // pixel format information
    int rsize,gsize,bsize;     // ditto
    
    int vsync;                 // vertal sync flag
    
    bool MMX;                  // set to true if we're allowed to use it
    
    SDL_Surface *pMainsurface;
    
    void GetPixelFormat();       // sets rpos, rsize, etc...
    
    void DestroySurfaces();
    
public:
    GrDriver();
    ~GrDriver();
    
    unsigned short trans_mask;
    
    int bpp;                     // BYTES (not bits) per pixel
    u8* screen;                // the virual screen.  Can be changed with SetRenderDest()

    SRect    clip;               // current clip rect. ;P
    
    u8 gamepal[768];           // the current palette, unmorphed
    u8 pal[768];               // the current palette, post-PaletteMorph
    
    // initialization/etc...
    bool Init(int x,int y,int bpp,bool fullscreen); // starts the whole thing up
    bool SetMode(int x,int y,int bpp,bool fullscreen);        // changes the resolution/full-screenedness
    void ShutDown();
    void VSync(bool on); // turns vsync on/off
    
    // colour stuff
    unsigned int Conv8(int c);           // 8 bit colour -> screen pixel
    unsigned int Conv8(u8* pal,int c);  // same, but with palette stipulated
    unsigned int Conv16(int c); // 5:6:5 pixel -> screen pixel
    unsigned int PackPixel(int r,int g,int b); // 8:8:8 pixel -> screen pixel
    void UnPackPixel(int c,int& r,int& g,int& b); // screen pixel -> 8:8:8 pixel
    int  SetPalette(u8* p); // char[768]
    int  GetPalette(u8* p); // ditto
    int  InitLucentLUT(u8* data); // initializes the 8bit lookup table
    void CalcLucentLUT(int lucency); // set the lucency table up
    
    // accessors
    int XRes();                  // the width of the actual screen
    int YRes();
    
    int scrx,scry;               // clip width/height
    
    bool IsFullScreen();
    char* DriverDesc();
    
    void Clear();
    
    //   blits
    // opaque blits
    void  CopySprite(int x,int y,int width,int height,u8* src);
    void TCopySprite(int x,int y,int width,int height,u8* src);
    void  ScaleSprite(int x,int y,int iwidth,int iheight,int dwidth,int dheight,u8* src);
    void TScaleSprite(int x,int y,int iwidth,int iheight,int dwidth,int dheight,u8* src);
    void RotScale(int posx,int posy,int width,int height,float angle,float scale, u8* src);
    void  WrapBlit(int x,int y,int width,int height, u8* src);
    void TWrapBlit(int x,int y,int width,int height, u8* src);
    void BlitStipple(int x,int y,int colour);
    // silhouette
    
    inline void SetPixelLucent(u16* dest,int c,int lucentmode);
    // translucent blits
    void  CopySpriteLucent(int x,int y,int width,int height,u8* src,int lucentmode);
    void TCopySpriteLucent(int x,int y,int width,int height,u8* src,int lucentmode);
    void  ScaleSpriteLucent(int x,int y,int iwidth,int iheight,int dwidth,int dheight,u8* src,int lucent);
    void TScaleSpriteLucent(int x,int y,int iwidth,int iheight,int dwidth,int dheight,u8* src,int lucent);
    void RotScaleLucent(int posx,int posy,int width,int height,float angle,float scale, u8* src,int lucent);
    void WrapBlitLucent(int x,int y,int width,int height, u8* src,int lucent);
    void TWrapBlitLucent(int x,int y,int width,int height, u8* src,int lucent);
    
    // Primatives
    void SetPixel(int x,int y,int colour,int lucent);
    int  GetPixel(int x,int y);
    void HLine(int x,int y,int x2,int colour,int lucent);
    void VLine(int x,int y,int y2,int colour,int lucent);
    void Line(int x1,int y1,int x2,int y2,int colour,int lucent);
    void Rect(int x1,int y1,int x2,int y2,int colour,int lucent);
    void RectFill(int x1,int y1,int x2,int y2,int colour,int lucent);
    void Circle(int x,int y,int radius,int colour,int lucent);
    void CircleFill(int x,int y,int radius,int colour,int lucent);
    
    // polygon crap
    void FlatPoly(int x1,int y1,int x2,int y2,int x3,int y3,int colour);
    
protected:
    void tmaphline(int x1, int x2, int y, int tx1, int tx2, int ty1, int ty2,int texw,int texh,u8* image);
public:
    void TMapPoly(int x1, int y1, int x2, int y2, int x3, int y3,
        int tx1, int ty1, int tx2, int ty2, int tx3, int ty3,
        int tw, int th, u8 *img);
    
    void Mask(u8* src,u8* mask, int width,int height, u8* dest);
    void Silhouette(int width,int height,u8* src,u8*dest,int colour);
    void ChangeAll(int width,int height,u8* src,int srccolour,int destcolour);
    
    void ShowPage();
    
    // weird rendering magic stuff ;)
    void SetClipRect(struct SRect& clip);
    void RestoreRenderSettings();
    void SetRenderDest(int x,int y,u8* dest);
    
protected:
    int morph_step(int S, int D, int mix, int light);
public:
    void PaletteMorph(int r,int g,int b,int percent,int intensity);
};

#endif
