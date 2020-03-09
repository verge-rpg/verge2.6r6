// tSB's SDL video crap for v2.6

// Portability: win32 and linux
// This is designed to work in a manner as close to the old DOS driver as conceivably possible.
//
// All of the clipping coords are inclusive.      (0, 0)-(319, 199) means that 319 is the last pixel to write on the X axis, etc...

#include "verge.h"
#include "vtypes.h"
#include "SDL.h"
#include "2xsai.h"
#include "videofilter.h"

#include <math.h>                       // for RotScale

#if defined(WIN32) && defined(_MSVC)
// Comment this to use all the C blitters
#   define GR_ASM
#   include <windows.h>    // CPUID
#endif

bool IsMMX();       // prototype

GrDriver::GrDriver()
    : screen(0)
    , xres(0)
    , yres(0)
    , fullScreen(false)
    , rsize(0), rpos(0)
    , gsize(0), gpos(0)
    , bsize(0), bpos(0)
    , MMX(false)
    , mainSurface(0)
    , trans_mask(0)
    , vsync(false)
{
}

GrDriver::~GrDriver()
{
    ShutDown();
}

// if we're fullscreen, then offsurf is attached to the main surface, and we simply flip() in ShowPage.
// if not, then offsurf is just another surface (at the requested colour depth) then, we can blit it to
// the primary and not have to worry about the desktop colour depth. ^_^
// returns 1 on success
// x and y are the resolution, c is the colour depth (in bits per pixel) fullscr is true if we aren't running windowed
bool GrDriver::Init(int x, int y, int c, bool fullscr, VideoFilter* filter)
{
    MMX = IsMMX();                         // what the hell, get a jumpstart. ;)
    
    if (MMX)
        Log::Write("MMX enabled");
    else
        Log::Write("MMX not detected");

    #ifdef VERGE_VIDEOFILTERS_ENABLED
    extern bool mmx_cpu; // bleh.
    mmx_cpu = MMX; // tell the 2xSaI stuff.
    #endif
    
    // bounds checking
    if (c != 8 && c != 16)  return false;   // TODO: 24/32bit support?
    if (x <= 0 || y <=  0)  return false;   // a mode with pixels on it would be preferable

    // first off, we don't care about the filter in 8bpp mode (yet)
    if (c == 8)
    {
        delete filter;
        filter = new Filter8bpp();
    }
    else if (filter == 0)
        filter = new FilterNone();

    _filter = filter;

    bpp = c/8;  // bpp is bytes per pixel, c is in bits.  Adjust.
    xres = x;    yres = y;
    
    return SetMode(x, y, c, fullscr);
}

bool GrDriver::SetMode(int x, int y, int c, bool fs)
{
    // TODO: Why doesn't switching to/from windowed mode work?
    DestroySurfaces();

    _filter->resize(x, y);

    mainSurface = SDL_SetVideoMode(x * _filter->scale(), y * _filter->scale(), _filter->depth(), SDL_DOUBLEBUF | (fs ? SDL_FULLSCREEN : 0) | (_filter->depth() == 8 ? SDL_HWPALETTE : 0));
    
    if (!mainSurface)
        return false;
    
    xres = x?x:xres;      yres = y?y:yres;      bpp = (c == 8 ? 1 : 2); fullScreen = fs;
   
    trueScreen = _filter->getBackBuffer();

    RestoreRenderSettings();

    if (c != 8)
    {
        // init hicolour stuff

        if (!_filter)
            GetPixelFormat();   // use the hardware pixel format
        else
        {
            // filters always use 16bpp
            rsize = 5;  rpos = 11;
            gsize = 6;  gpos = 5;
            bsize = 5;  bpos = 0;
            lucentmask = ~((1 << rpos) | (1 << gpos) | (1 << bpos));
        }
        
        trans_mask = PackPixel(255, 0, 255);
    }

    return true;
}

void GrDriver::DestroySurfaces()
{
    // heh, sure are a lot of surfaces to destroy
}

void GrDriver::ShutDown()
{
    DestroySurfaces();
}

extern u8 cpubyte;

void GrDriver::ShowPage()
{
    RenderGUI(); // gah! --tSB
    
    cpubyte = PFLIP;

    if (morphlut) // bleh
    {
#if !defined(GR_ASM) || 1
        // Less than ideal, as we're destructively altering the backbuffer.
        int i = xres * yres;
        u16* p = reinterpret_cast<u16*>(trueScreen);
        while (i--)
        {
            *p = morphlut[*p];
            p++;
        }
#else
        // TODO: assembly impl?
#endif
    }

    SDL_LockSurface(mainSurface);
    u8* pPixels=reinterpret_cast<u8*>(mainSurface->pixels);

    {
        _filter->flip(reinterpret_cast<u8*>(mainSurface->pixels), mainSurface->pitch);
    }
    
    SDL_UnlockSurface(mainSurface);
    
    SDL_Flip(mainSurface);
    
    cpubyte = ETC;
}

// accessors
int GrDriver::XRes()
{
    return xres;
}

int GrDriver::YRes()
{
    return yres;
}

bool GrDriver::IsFullScreen()
{
    return fullScreen;
}

std::string GrDriver::DriverDesc()
{
    return driverDesc;
}

void GrDriver::Clear()
{
    memset(screen, 0, xres*yres*bpp);
}

void GrDriver::VSync(bool on)
{
}

// ===================================== OPAQUE BLITS =====================================

void GrDriver::CopySprite(int x, int y, int width, int height, u8* src)
{
    int xl, yl, xs, ys;
    
    if (screen==NULL) return;
    
    cpubyte = RENDER;
    xl = width;  yl = height;
    xs = ys = 0;
    if (x>=clip.right || y>=clip.bottom || x+xl<=clip.left || y+yl<=clip.top)
        return;
    
    if (x+xl-1 >= clip.right ) xl = clip.right -x+1;
    if (y+yl-1 >= clip.bottom) yl = clip.bottom-y+1;
    if (x<clip.left) { xs = clip.left-x;         xl-=xs;   x = clip.left; }
    if (y<clip.top ) { ys = clip.top -y;         yl-=ys;   y = clip.top ; }
    
    if (bpp==2) // blit for hicolour
    {
        u16 *s16, *d16;
        
        s16=(u16*)(src) + ((ys * width) + xs);
        d16=(u16*)(screen) + y * scrx + x;
#ifndef GR_ASM
        xl<<=1;
        while (yl)
        {
            memcpy(d16, s16, xl);
            s16+=width;
            d16+=scrx;
            yl--;
        }
#else
        // Registers:
        // esi: source
        // edi: dest
        // ecx: x loop counter
        // edx: y loop counter
        // eax: source pointer increment
        // ebx: dest pointer increment
        
        // The ops that are paired are the ones I'm hoping will fit into the u & v pipes
        
        // Teeny bit faster than the C version.  The MMX version is a teeny bit faster yet.
        
        int sx = scrx;
        
        //*
        if (MMX)
        {
            
            int numquads = xl/4;                        // (widthinpixels)*bpp/8  (where bpp = 2 here)
            int leftover = xl%4;
            
            __asm
            {
                mov     esi, s16                // esi = s16
                mov     edi, d16                // edi = d16
                
                mov     eax, width              // eax = width
                mov     ebx, sx                 // ebx = sx
                
                sub     eax, xl                 // eax = width-xl
                
                sub     ebx, xl                 // ebx = sx-xl
                shl     eax, 1                  // eax=(width-xl)*2 (u8s, not pixels)
                
                shl     ebx, 1                  // ebx=(sx-xl)*2
                
                mov     edx, yl                 // edx = yl
                    
yloop16mmx:
                mov     ecx, numquads
                jcxz    leftovers16mmx
                    
xloop16mmx:
                movq   mm0, [esi]       // grab a bunch of pixels
                add    esi, 8
                
                movq   [edi], mm0       // dump 'em back
                add    edi, 8           // inc the source/dest pointers
                
                loop    xloop16mmx
                    
leftovers16mmx:
                mov     ecx, leftover
                jcxz    endline16mmx
                
                rep     movsw           // copy any odd pixels
                    
endline16mmx:
                add     esi, eax        // move the pointers down to the beginning of the next line
                add     edi, ebx
                
                dec     edx
                jnz     yloop16mmx
                
                emms                            
            }
        }
        else      //*/
            __asm
        {
            mov         esi, s16
            mov         edi, d16
            
            mov         eax, width      // eax = width
            mov         ebx, sx         // ebx = scrx
            
            sub         eax, xl         // eax = width-xl
            
            sub         ebx, xl         // ebx = scrx-xl
            shl         eax, 1          // eax=(width-xl)*2; (2 bpp)
            
            shl         ebx, 1          // ebx=(scrx-xl)*2; (2bpp)
            
            mov         edx, yl         // dx = y length
yloop16:
            mov         ecx, xl         // ecx = xl (in pixels)
            rep         movsw           // copy a line
            
            add         esi, eax
            add         edi, ebx
            
            dec         edx
            jnz         yloop16
        }
#endif // GR_ASM
    }
    else // 8bit blit
    {
        u8 *s8, *d8;
        s8 = src+(ys*width+xs);
        d8 = screen+(y*scrx+x);

        // asm is pointless here, as intel's memcpy implementation is pretty slick
        // to be honest, it's pretty pointless in the 16bpp implementation as well.
        for (; yl; yl--)
        {
            memcpy(d8, s8, xl);
            s8+=width;
            d8+=scrx;
        }
    }
}

void GrDriver::TCopySprite(int x, int y, int width, int height, u8* src)
{
    int xl, yl, xs, ys;
    int srci, desti; // source inc, dest inc
    
    if (screen==NULL) return;
    
    cpubyte = RENDER;
    xl = width;
    yl = height;
    xs = ys = 0;
    if (x>=clip.right || y>=clip.bottom || x+xl<=clip.left || y+yl<=clip.top)
        return;
    
    if (x+xl-1> clip.right ) xl = clip.right -x+1;
    if (y+yl-1 > clip.bottom) yl = clip.bottom-y+1;
    if (x<clip.left) { xs = clip.left-x;         xl-=xs;   x = clip.left; }
    if (y<clip.top ) { ys = clip.top -y;         yl-=ys;   y = clip.top ; }
    
    desti = scrx-xl;
    srci = width-xl;
    
    if (bpp>1) // blit for hicolour
    {
        u16 *s16, *d16;
        
        s16=(u16*)(src)+ys*width+xs;
        d16=(u16*)(screen)+y*scrx+x;
#ifndef GR_ASM
        for (; yl>0; yl--)
        {
            int a = xl;
            while (a--)
            {
                if (*s16!=trans_mask)
                    *d16=*s16;
                d16++; s16++;
            }
            d16+=desti;
            s16+=srci;
        }
#else
        int sx = scrx;
        u16 t = trans_mask;
        /*
        Registers:
        esi - Source ptr
        edi - dest ptr
        eax - source increment / current pixel (whatever)
        ebx - dest increment
        ecx - x loop counter
        edx - y loop counter
        */
        __asm
        {
            mov         esi, s16
            mov         edi, d16
        
            mov         eax, srci               // eax = srci
            mov         ebx, desti              // ebx = desti
        
            shl         eax, 1                  // u8s, not pixels
            shl         ebx, 1                  // ditto
        
            mov         edx, yl
yloop16:
            mov         ecx, xl
            push        ax                      // we need ax for awhile
                
xloop16:
                    mov         ax, [esi]       // grab a pixel
                    add         esi, 2          // increment the source pointer
            
                    cmp         ax, t
                    je          nodraw16
            
                    mov         [edi], ax
                
nodraw16:
                add     edi, 2
                loop    xloop16
            
                pop     ax
                add     esi, eax
                add     edi, ebx
            
            dec         edx
            jnz         yloop16
            
            //end16:
        }
#endif
    }
    else // 8bit blit
    {
        u8 *s8, *d8;
        s8 = src+ys*width+xs;
        d8 = screen+y*scrx+x;
#ifndef GR_ASM
        for (; yl; yl--)
        {
            x = xl;
            while (x--)
            {
                if (*s8)
                    *d8=*s8;
                d8++; s8++;             
            }
            d8+=desti;
            s8+=srci;
        }
#else    
        /* registers
        esi - source pointer
        edi - dest pointer
        edx - y loop counter
        ecx - x loop counter
        ebx - desti (above)
        eax - srci (above), and the current pixel
        
          Notes:  MMX isn't feasable for nonopaque blits, so don't even consider it.
          This one improves the framerate by about %8 on my system. (pretty good, IMHO)
        */
        int sx = scrx;                  // can't access class members in inline asm.
        __asm
        {
            mov         esi, s8
            mov         edi, d8
            
            mov         edx, yl         // edx = yl
            mov         ebx, desti
            mov         eax, srci
                
yloop8:
            mov         ecx, xl         // set up the x loop counter
            push        eax             // we need this register for awhile
                
xloop8:
            mov         al, [esi]       // faster than lodsb o_O
            inc         esi
            
            cmp         al, 0           // is it the transparent pixel value?
            je          nodraw8         // yes.  Skip it
            
            mov         [edi], al       // faster than stosb O_o (even when you take the inc into account)
nodraw8:
            inc         edi
            loop        xloop8
            
            pop         eax             // get our source incrementor back
            add         edi, ebx        // d8+=desti;
            add         esi, eax        // s8+=srci;
            
            dec         edx             // dec the y loop counter
            jnz         yloop8
        }
#endif
    }
    cpubyte = ETC;
}

// Seeing as how I spent the day I got off thanks to Rememberance day porting this code segment,
// I hereby dedicate ScaleSprite to all who died in the War(s).
void GrDriver::ScaleSprite(int x, int y, int iwidth, int iheight, int dwidth, int dheight, u8* src)
{
    int xerr, yerr;
    int xerr_start, yerr_start;
    int xadj, yadj;
    int xl, yl, xs, ys;
    
    cpubyte = RENDER;
    
    if (dwidth < 1 || dheight < 1)
        return;
    
    xl = dwidth;
    yl = dheight;
    xs = ys = 0;
    if (x > clip.right || y > clip.bottom
        || x + xl < clip.left || y + yl < clip.top)
        return;
    
    if (x+xl-1 > clip.right)   xl = clip.right - x+1;
    if (y+yl-1 > clip.bottom)  yl = clip.bottom - y+1;
    
    if (x < clip.left) { xs = clip.left - x; xl -= xs; x = clip.left; }
    if (y < clip.top)  { ys = clip.top - y;  yl -= ys; y = clip.top;  }
    
    xadj = (iwidth << 16)/dwidth;
    yadj = (iheight << 16)/dheight;
    xerr_start = xadj * xs;
    yerr_start = yadj * ys;
    
    yerr = yerr_start & 0xffff;
    
    if (bpp==1)
    {
        u8 *s8, *d8;
        
        s8 = src+((yerr_start>>16)*iwidth);
        d8 = screen+y*scrx+x;

#ifndef GR_ASM
        for (int i = 0; i < yl; i += 1)
        {
            xerr = xerr_start;
            for (int j = 0; j < xl; j += 1)
            {
                u8 c = s8[(xerr >> 16)];
                d8[j] = c;//translucency_table[d[j] | (c << 8)];
                xerr += xadj;
            }
            d8   += scrx;
            yerr += yadj;
            s8   += (yerr >> 16)*iwidth;
            yerr &= 0xffff;
        }
#else
            /*
            Registers:
            esi - source pointer
            edi - dest pointer
            al  - source pixel
            ebx - y loop counter
            ecx - x loop counter
            edx - source offset index/temp register for math-type stuff
            */
            int screenx = scrx;
            
            if (!yl || !xl) return;     // caca
            
            // this is by far the most complex blit I've ever done in ASM.  I'm proud. :>
            // Way, way faster than the C version, too.
            __asm
            {
                mov             esi, s8
                mov             edi, d8
                mov             ebx, yl
yloop8:
                    xor         ecx, ecx
                    mov         edx, xerr_start
                    mov         xerr, edx
xloop8:
                        mov             edx, xerr
                        shr             edx, 16
                
                        mov             al, [esi+edx]
                        mov             [edi+ecx], al
                
                        mov             edx, xadj
                        add             xerr, edx
                
                        inc             ecx
                        cmp             ecx, xl
                        jl      xloop8
                
                        add             edi, screenx
                
                        mov             edx, yadj
                        add             edx, yerr               // edx = yadj+yerr
                        mov             yerr, edx               // yerr = edx  (yerr+=yadj, edx = yerr;)
                
                        mov             eax, iwidth             // eax = iwidth
                        shr             edx, 16                 // edx = yerr>>16
                
                        mul             dx                              // eax=(yerr>>16)*iwidth
                        add             esi, eax                        // esi+=(yerr>>16)*iwidth
                
                        and             yerr, 0xFFFF            // yerr&=0xFFFF;
                
                dec             ebx
                jnz             yloop8
            }
#endif
    }
    else
    {
        u16 *s16, *d16;
        
        s16=(u16*)(src)+(yerr_start>>16)*iwidth;
        d16=(u16*)(screen)+y*scrx+x;
        
#ifndef GR_ASM
        for (int i = 0; i < yl; i += 1)
        {
            xerr = xerr_start;
            for (int j = 0; j < xl; j += 1)
            {
                u16 c = s16[(xerr >> 16)];
                d16[j] = c;
                xerr += xadj;
            }
            d16  += scrx;
            yerr += yadj;
            s16  += (yerr >> 16)*iwidth;
            yerr &= 0xffff;
        }
#else
        /*
        Registers:
        esi - source pointer
        edi - dest pointer
        ax  - source pixel
        ebx - y loop counter
        ecx - x loop counter
        edx - source offset index/temp register for math-type stuff
        */
        int screenx = scrx;
        
        if (!yl || !xl) return; // caca
        
        __asm
        {
            mov         esi, s16
            mov         edi, d16
            mov         ebx, yl
yloop16:
                xor             ecx, ecx
                mov             edx, xerr_start
                mov             xerr, edx
xloop16:
                    mov         edx, xerr
                    shr         edx, 15                 // >>16, but *2 since we want a word-sized offset
                    and         edx, ~1                 // strip the low bit
                
                    mov         ax, [esi+edx]   // grab a pixel
                
                    shl         ecx, 1                  // we need a word sized offset for a sec, double ecx
                    mov         [edi+ecx], ax
                    shr         ecx, 1                  // restore our loop counter
                
                    mov         edx, xadj
                    add         xerr, edx
                
                    inc         ecx
                    cmp         ecx, xl
                    jl  xloop16
                
                    add         edi, screenx
                    add         edi, screenx            // word sized offset (again!)
                
                    mov         edx, yadj
                    add         edx, yerr               // edx = yadj+yerr
                    mov         yerr, edx               // yerr = edx  (yerr+=yadj, edx = yerr;)
                
                    mov         eax, iwidth             // eax = iwidth
                    shr         edx, 16                 // edx = yerr>>16
                
                    mul         dx                              // eax=(yerr>>16)*iwidth
                
                    shl         eax, 1                  // need a word-sized offset
                    add         esi, eax                        // esi+=(yerr>>16)*iwidth
                
                    and         yerr, 0xFFFF            // yerr&=0xFFFF;
                
            dec         ebx
            jnz         yloop16
        }
#endif
    }
}

void GrDriver::TScaleSprite(int x, int y, int iwidth, int iheight, int dwidth, int dheight, u8* src)
{
    int i, j;
    int xerr, yerr;
    int xerr_start, yerr_start;
    int xadj, yadj;
    int xl, yl, xs, ys;
    int c;
    
    cpubyte = RENDER;
    
    if (dwidth < 1 || dheight < 1)
        return;
    
    xl = dwidth;
    yl = dheight;
    xs = ys = 0;
    if (x > clip.right || y > clip.bottom
        || x + xl < clip.left || y + yl < clip.top)
        return;
    
    if (x+xl-1 > clip.right)   xl = clip.right - x+1;
    if (y+yl-1 > clip.bottom)  yl = clip.bottom - y+1;
    
    if (x < clip.left) { xs = clip.left - x; xl -= xs; x = clip.left; }
    if (y < clip.top) { ys = clip.top - y; yl -= ys; y = clip.top; }
    
    xadj = (iwidth << 16)/dwidth;
    yadj = (iheight << 16)/dheight;
    xerr_start = xadj * xs;
    yerr_start = yadj * ys;
    
    yerr = yerr_start & 0xffff;
    
    if (bpp==1)
    {
        u8 *s8, *d8;
        
        s8 = src+(ys*iwidth);
        d8 = screen+y*scrx+x;
        
        for (i = 0; i < yl; i += 1)
        {
            xerr = xerr_start;
            for (j = 0; j < xl; j += 1)
            {
                c = s8[(xerr >> 16)];
                if (c)
                    d8[j]=c;//translucency_table[d[j] | (c << 8)];
                xerr += xadj;
            }
            d8   += scrx;
            yerr += yadj;
            s8   += (yerr >> 16)*iwidth;
            yerr &= 0xffff;
        }
    }
    else
    {
        u16 *s16, *d16;
        
        s16=(u16*)(src)+(yerr_start>>16)*iwidth;
        d16=(u16*)(screen)+y*scrx+x;
        
        for (i = 0; i < yl; i += 1)
        {
            xerr = xerr_start;
            for (j = 0; j < xl; j += 1)
            {
                c = s16[(xerr >> 16)];
                if (c!=trans_mask)
                    d16[j] = c;
                xerr += xadj;
            }
            d16  += scrx;
            yerr += yadj;
            s16  += (yerr >> 16)*iwidth;
            yerr &= 0xffff;
        }
    }
}

void GrDriver::RotScale(int posx, int posy, int width, int height, float angle, float scale, u8* src)
{
    // new! shamelessly ripped off from alias.zip
    // except the atan2 stuff which i had to make up myself AEN so there :p
    
    int xs, ys, xl, yl;
    int sinas, cosas, xc, yc, srcx, srcy, x, y, tempx, tempy, T_WIDTH_CENTER, T_HEIGHT_CENTER, W_WIDTH_CENTER, W_HEIGHT_CENTER, W_HEIGHT, W_WIDTH;
    u16 pt;
    float ft;
    
    ft=(float)atan2((float)width, (float)height);
    
    T_WIDTH_CENTER = width>>1;
    T_HEIGHT_CENTER = height>>1;
    W_WIDTH=(int)((float)width/scale*sin(ft) + (float)height/scale*cos(ft));
    W_HEIGHT = W_WIDTH;
    W_HEIGHT_CENTER = W_HEIGHT>>1;
    W_WIDTH_CENTER = W_HEIGHT_CENTER; //W_WIDTH/2;
    
    sinas=(int)(sin(-angle)*65536*scale);
    cosas=(int)(cos(-angle)*65536*scale);
    
    xc = T_WIDTH_CENTER*65536 - (W_HEIGHT_CENTER*(cosas+sinas));
    yc = T_HEIGHT_CENTER*65536 - (W_WIDTH_CENTER*(cosas-sinas));
    posx-=W_WIDTH_CENTER;
    posy-=W_HEIGHT_CENTER;
    
    // clipping
    if (W_WIDTH<2 || W_HEIGHT<2) return;
    xl = W_WIDTH;
    yl = W_HEIGHT;
    xs = ys = 0;
    if (posx>clip.right || posy>clip.bottom || posx+xl<clip.left || posy+yl<clip.top)
        return;
    if (posx+xl-1 > clip.right) xl = clip.right-posx+1;
    if (posy+yl-1 > clip.bottom) yl = clip.bottom-posy+1;
    if (posx<clip.left)
    {
        xs = clip.left-posx;
        xl-=xs;
        posx = clip.left;
        
        xc+=cosas*xs; // woo!
        yc-=sinas*xs;
    }
    if (posy<clip.top)
    {
        ys = clip.top-posy;
        yl-=ys;
        posy = clip.top;
        
        xc+=sinas*ys; // woo!
        yc+=cosas*ys;
    }
    
    
    if (bpp==1)
    {
        u8 *s8, *d8;
        d8 = screen+posx+posy*scrx;
        s8 = src;
        for (y = 0; y<yl; y++)
        {
            srcx = xc;
            srcy = yc;
            
            for (x = 0; x<xl; x++)
            {
                tempx=(srcx>>16);
                tempy=(srcy>>16);
                
                if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
                {
                    pt = s8[tempx+tempy*width];
                    if (pt)
                        d8[x]=(u8)pt;//dest[x]=translucency_table[(pt<<8)|dest[x]];
                }
                
                srcx+=cosas;
                srcy-=sinas;
            }
            
            d8+=scrx;
            
            xc+=sinas;
            yc+=cosas;
        }
    }
    else
    {
        u16 *s16, *d16;
        d16=(u16*)screen+posx+posy*scrx;
        s16=(u16*)src;
        for (y = 0; y<yl; y++)
        {
            srcx = xc;
            srcy = yc;
            
            for (x = 0; x<xl; x++)
            {
                tempx=(srcx>>16);
                tempy=(srcy>>16);
                
                if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
                {
                    pt = s16[tempx+tempy*width];
                    if (pt!=trans_mask)
                        d16[x]=pt;//dest[x]=translucency_table[(pt<<8)|dest[x]];
                }
                
                srcx+=cosas;
                srcy-=sinas;
            }
            
            d16+=scrx;
            
            xc+=sinas;
            yc+=cosas;
        }
    }
}

void GrDriver::WrapBlit(int x, int y, int width, int height, u8* src)
{
    int cur_x, sign_y;
    
    if (width < 1 || height < 1)
        return;
    
    x %= width, y %= height;
    sign_y = 0 - y;
    
    for (; sign_y < scry; sign_y += height)
    {
        for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
            CopySprite(cur_x, sign_y, width, height, src);
    }
}

void GrDriver::TWrapBlit(int x, int y, int width, int height, u8* src)
{
    int cur_x, sign_y;
    
    if (width < 1 || height < 1)
        return;
    
    x %= width, y %= height;
    sign_y = 0 - y;
    
    for (; sign_y < scry; sign_y += height)
    {
        for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
            TCopySprite(cur_x, sign_y, width, height, src);
    }
}

// ===================================== TRANSLUCENT BLITS =====================================

// TODO: annihilate this in favour of a templatized functor solution.
// Should be a gazillion times faster.
inline void GrDriver::SetPixelLucent(u16* dest, int c, int lucentmode)
// only for hicolour.  It should be noted that this simply accepts a pointer to 
// dest, insted of x and y coordinates.  This is simply because the blits 
// themselves can calculate dest more efficiently themselves.  (speed is good)
// TODO: ASM!  Everywhere, anywhere!
// Or not! (god I was a retard. -_-)
{
    int  rs, gs, bs; // rgb of source pixel
    int  rd, gd, bd; // rgb of dest pixel
    
    switch (lucentmode)
    {
    case 0: *dest = c; return; // wtf? oh well, return it anyway
    case 1: *dest=(u16)(((*dest&lucentmask)+(c&lucentmask))>>1); return; // ooh easy stuff.
    case 2: // variable lucency
        UnPackPixel(*dest, rs, gs, bs);
        UnPackPixel(c, rd, gd, bd);
        rd = lucentlut16[rd<<8|rs];
        gd = lucentlut16[gd<<8|gs];
        bd = lucentlut16[bd<<8|bs];
        *dest = PackPixel(rd, gd, bd);             
        return;
    case 3: // addition
        UnPackPixel(*dest, rs, gs, bs);
        UnPackPixel(c, rd, gd, bd);
        rs+=rd; gs+=gd; bs+=bd;
        if (rs>255) rs = 255;
        if (gs>255) gs = 255;
        if (bs>255) bs = 255;
        *dest = PackPixel(rs, gs, bs);
        return;
    case 4: // subtraction
        UnPackPixel(*dest, rs, gs, bs);
        UnPackPixel(c, rd, gd, bd);
        rs-=rd; gs-=gd; bs-=bd;
        if (rs<0) rs = 0;
        if (gs<0) gs = 0;
        if (bs<0) bs = 0;
        *dest = PackPixel(rs, gs, bs);
        return;
        /* this is quasi-slick, IMO.
        Instead of dividing the colour values, it simply blends them with black,
        using the variable lucency table.  In effect, doing the same thing, but fasta! --tSB*/
    case 5: UnPackPixel(*dest, rs, gs, bs);
        UnPackPixel(c, rd, gd, bd);
        rd = lucentlut16[rd<<8];
        gd = lucentlut16[gd<<8];
        bd = lucentlut16[bd<<8];
        rs+=rd; gs+=gd; bs+=bd;
        if (rs>255) rs = 255;
        if (gs>255) gs = 255;
        if (bs>255) bs = 255;
        *dest = PackPixel(rs, gs, bs);            
        return;
        
    case 6: UnPackPixel(*dest, rs, gs, bs);
        UnPackPixel(c, rd, gd, bd);
        rd = lucentlut16[rd<<8];
        gd = lucentlut16[gd<<8];
        bd = lucentlut16[bd<<8];
        rs-=rd; gs-=gd; bs-=bd;
        if (rs<0) rs = 0;
        if (gs<0) gs = 0;
        if (bs<0) bs = 0;
        *dest = PackPixel(rs, gs, bs);            
        return;
    }
}

namespace
{
    struct Opaque
    {
        static inline u16 Blend(u16 src, u16 dest) { return dest; }
    };

    template <class BlendPolicy>
    struct Matte
    {
        static inline u16 Blend(u16 src, u16 dest) { return src == gfx.trans_mask ? dest : BlendPolicy::Blend(src, dest); }
    };

    struct AlphaHalf
    {
        static inline u16 Blend(u16 src, u16 dest) 
        { 
            return (u16)(((dest & gfx.lucentmask) + (src & gfx.lucentmask)) >> 1); 
        }
    };

    struct Alpha
    {
        static inline u16 Blend(u16 src, u16 dest)
        {
            int rs, gs, bs;
            int rd, gd, bd;
            gfx.UnPackPixel(dest, rs, gs, bs);
            gfx.UnPackPixel(src, rd, gd, bd);
            rd = gfx.lucentlut16[rd << 8 | rs];
            gd = gfx.lucentlut16[gd << 8 | gs];
            bd = gfx.lucentlut16[bd << 8 | bs];
            return gfx.PackPixel(rd, gd, bd);              
        }
    };

    struct Addition
    {
        static inline u16 Blend(u16 src, u16 dest)
        {
            int rs, gs, bs;
            int rd, gd, bd;
            gfx.UnPackPixel(dest, rs, gs, bs);
            gfx.UnPackPixel(src , rd, gd, bd);
            rs += rd; gs += gd; bs += bd;
            if (rs > 255) rs = 255;
            if (gs > 255) gs = 255;
            if (bs > 255) bs = 255;
            return gfx.PackPixel(rs, gs, bs);
        }
    };

    struct Subtraction
    {
        static inline u16 Blend(u16 src, u16 dest)
        {
            int rs, gs, bs;
            int rd, gd, bd;
            gfx.UnPackPixel(dest, rs, gs, bs);
            gfx.UnPackPixel(src , rd, gd, bd);
            rs -= rd; gs -= gd; bs -= bd;
            if (rs < 0) rs = 0;
            if (gs < 0) gs = 0;
            if (bs < 0) bs = 0;
            return gfx.PackPixel(rs, gs, bs);
        }
    };

    struct VarAddition
    {
        static inline u16 Blend(u16 src, u16 dest)
        {
            int rs, gs, bs;
            int rd, gd, bd;

            gfx.UnPackPixel(dest, rs, gs, bs);
            gfx.UnPackPixel(src , rd, gd, bd);
            rd = gfx.lucentlut16[rd<<8];
            gd = gfx.lucentlut16[gd<<8];
            bd = gfx.lucentlut16[bd<<8];
            rs += rd; gs += gd; bs += bd;
            if (rs > 255) rs = 255;
            if (gs > 255) gs = 255;
            if (bs > 255) bs = 255;
            return gfx.PackPixel(rs, gs, bs);             
        }
    };

    struct VarSubtraction
    {
        static inline u16 Blend(u16 src, u16 dest)
        {
            int rs, gs, bs;
            int rd, gd, bd;

            gfx.UnPackPixel(dest, rs, gs, bs);
            gfx.UnPackPixel(src , rd, gd, bd);
            rd = gfx.lucentlut16[rd<<8];
            gd = gfx.lucentlut16[gd<<8];
            bd = gfx.lucentlut16[bd<<8];
            rs -= rd; gs -= gd; bs -= bd;
            if (rs < 0) rs = 0;
            if (gs < 0) gs = 0;
            if (bs < 0) bs = 0;
            return gfx.PackPixel(rs, gs, bs);             
        }
    };
}

template <class BlendPolicy>
void GrDriver::CopySprite16(int x, int y, int width, int height, u8* src)
{
    const SRect& clip = gfx.clip;
    u16* screen = (u16*)gfx.screen;
    int xl, yl, xs, ys;
    int a;
    
    cpubyte = RENDER;
    xl = width;
    yl = height;
    xs = ys = 0;
    if (x>clip.right || y>clip.bottom || x+xl<clip.left || y+yl<clip.top)
        return;
    
    if (x+xl-1 > clip.right ) xl = clip.right  - x + 1;
    if (y+yl-1 > clip.bottom) yl = clip.bottom - y + 1;
    if (x<clip.left) { xs = clip.left - x;       xl -= xs;   x = clip.left; }
    if (y<clip.top ) { ys = clip.top  - y;       yl -= ys;   y = clip.top ; }
    
    u16* s16 = (u16*)(src) + ys * width + xs;
    u16* d16 = screen + y * gfx.scrx+x;
    for (; yl; yl--) // TODO: asm.  Except not.
    {
        for (a = 0; a<xl; a++)
            d16[a] = BlendPolicy::Blend(s16[a], d16[a]);

        s16 += width;
        d16 += gfx.scrx;
    }

    cpubyte = ETC;
}

template <class BlendPolicy>
void GrDriver::ScaleSprite16(int x, int y, int width, int height, int stretchwidth, int stretchheight, u8* src)
{
    const SRect& clip = gfx.clip;
    u16* screen = (u16*)gfx.screen;

    int i, j;
    int xerr, yerr;
    int xerr_start, yerr_start;
    int xadj, yadj;
    int xl, yl, xs, ys;
    int c;
    
    cpubyte = RENDER;
    
    if (stretchwidth < 1 || stretchheight < 1)
        return;
    
    xl = stretchwidth;
    yl = stretchheight;
    xs = ys = 0;
    if (x > clip.right || y > clip.bottom
        || x + xl < clip.left || y + yl < clip.top)
        return;
    
    if (x+xl-1 > clip.right)   xl = clip.right - x + 1;
    if (y+yl-1 > clip.bottom)  yl = clip.bottom - y + 1;
    
    if (x < clip.left) { xs = clip.left - x; xl -= xs; x = clip.left; }
    if (y < clip.top) { ys = clip.top - y; yl -= ys; y = clip.top; }
    
    xadj = (width << 16)/stretchwidth;
    yadj = (height << 16)/stretchheight;
    xerr_start = xadj * xs;
    yerr_start = yadj * ys;
    
    yerr = yerr_start & 0xffff;
    
    u16* s16 = (u16*)(src) + (yerr_start >> 16) * width;
    u16* d16 = screen + y * gfx.scrx + x;
    
    for (i = 0; i < yl; i += 1)
    {
        xerr = xerr_start;
        for (j = 0; j < xl; j += 1)
        {
            c = s16[(xerr >> 16)];
            d16[j] = BlendPolicy::Blend(c, d16[j]);
            xerr += xadj;
        }
        d16  += gfx.scrx;
        yerr += yadj;
        s16  += (yerr >> 16)*width;
        yerr &= 0xffff;
    }
}

template <class BlendPolicy>
void GrDriver::RotScaleSprite16(int posx, int posy, int width, int height, float angle, float scale, u8* src)
{
    // new! shamelessly ripped off from alias.zip
    // except the atan2 stuff which i had to make up myself AEN so there :p
    
    const SRect& clip = gfx.clip;
    u16* screen = (u16*)gfx.screen;

    int xs, ys, xl, yl;
    int sinas, cosas, xc, yc, x, y;
    float ft;
    
    ft=(float)atan2((float)width, (float)height);
    
    const int T_WIDTH_CENTER = width>>1;
    const int T_HEIGHT_CENTER = height>>1;
    const int W_WIDTH=(int)((float)width/scale*sin(ft) + (float)height/scale*cos(ft));
    const int W_HEIGHT = W_WIDTH;
    const int W_HEIGHT_CENTER = W_HEIGHT>>1;
    const int W_WIDTH_CENTER = W_HEIGHT_CENTER; //W_WIDTH/2;
    
    sinas=(int)(sin(-angle)*65536*scale);
    cosas=(int)(cos(-angle)*65536*scale);
    
    xc = T_WIDTH_CENTER*65536 - (W_HEIGHT_CENTER*(cosas+sinas));
    yc = T_HEIGHT_CENTER*65536 - (W_WIDTH_CENTER*(cosas-sinas));
    posx-=W_WIDTH_CENTER;
    posy-=W_HEIGHT_CENTER;
    
    // clipping
    if (W_WIDTH<2 || W_HEIGHT<2) return;
    xl = W_WIDTH;
    yl = W_HEIGHT;
    xs = ys = 0;
    if (posx>clip.right || posy>clip.bottom || posx+xl<clip.left || posy+yl<clip.top)
        return;
    if (posx+xl-1 > clip.right) xl = clip.right-posx+1;
    if (posy+yl-1 > clip.bottom) yl = clip.bottom-posy+1;
    if (posx<clip.left)
    {
        xs = clip.left-posx;
        xl-=xs;
        posx = clip.left;
        
        xc+=cosas*xs; // woo!
        yc-=sinas*xs;
    }
    if (posy<clip.top)
    {
        ys = clip.top-posy;
        yl-=ys;
        posy = clip.top;
        
        xc+=sinas*ys; // woo!
        yc+=cosas*ys;
    }
    
    u16 *s16, *d16;
    d16 = screen + posy * gfx.scrx + posx;
    s16 = (u16*)src;
    for (y = 0; y<yl; y++)
    {
        int srcx = xc;
        int srcy = yc;
        
        for (x = 0; x<xl; x++)
        {
            int tempx = (srcx>>16);
            int tempy = (srcy>>16);
            
            if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
                d16[x] = BlendPolicy::Blend(d16[x], s16[tempx+tempy*width]);
            
            srcx+=cosas;
            srcy-=sinas;
        }
        
        d16+=scrx;
        
        xc+=sinas;
        yc+=cosas;
    }
}

template <class BlendPolicy>
void GrDriver::WrapSprite16(int x, int y, int width, int height, u8* src)
{
    int cur_x, sign_y;
    
    if (width < 1 || height < 1)
        return;
    
    x %= width, y %= height;
    sign_y = 0 - y;
    
    while (sign_y < scry)
    {
        for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
            CopySprite16<BlendPolicy>(cur_x, sign_y, width, height, src);
        sign_y += height;
    }
}

void GrDriver::CopySpriteLucent(int x, int y, int width, int height, u8* src, int lucentmode)
{
    if (bpp > 1) // 16bpp
    {
        switch (lucentmode)
        {
        default:
        case 0: CopySprite16<Opaque>(x, y, width, height, src);  break;
        case 1: CopySprite16<AlphaHalf>(x, y, width, height, src); break;
        case 2: CopySprite16<Alpha>(x, y, width, height, src); break;
        case 3: CopySprite16<Addition>(x, y, width, height, src); break;
        case 4: CopySprite16<Subtraction>(x, y, width, height, src); break;
        case 5: CopySprite16<VarAddition>(x, y, width, height, src); break;
        case 6: CopySprite16<VarSubtraction>(x, y, width, height, src); break;
        }
        return;
    }

    int xl, yl, xs, ys;
    int a;
    
    if (screen==NULL) return;
    
    cpubyte = RENDER;
    xl = width;
    yl = height;
    xs = ys = 0;
    if (x>clip.right || y>clip.bottom || x+xl<clip.left || y+yl<clip.top)
        return;
    
    if (x+xl-1 > clip.right ) xl = clip.right -x+1;
    if (y+yl-1 > clip.bottom) yl = clip.bottom-y+1;
    if (x<clip.left) { xs = clip.left-x;         xl-=xs;   x = clip.left; }
    if (y<clip.top ) { ys = clip.top -y;         yl-=ys;   y = clip.top ; }
    
    u8 *s8, *d8;
    s8 = src+ys*width+xs;
    d8 = screen+y*scrx+x;
    
    for (; yl; yl--)
    {
        for (a = 0; a<xl; a++)
            d8[a] = lucentlut8[d8[a] | (s8[a] << 8)]; // wee
        s8+=width;
        d8+=scrx;
    }

    cpubyte = ETC;
}

void GrDriver::TCopySpriteLucent(int x, int y, int width, int height, u8* src, int lucentmode)
{
    if (bpp > 1) // 16bpp
    {
        switch (lucentmode)
        {
        default:
        case 0: CopySprite16<Matte<Opaque> >(x, y, width, height, src);  break;
        case 
            1: CopySprite16<Matte<AlphaHalf> >(x, y, width, height, src); break;
        case 2: CopySprite16<Matte<Alpha> >(x, y, width, height, src); break;
        case 3: CopySprite16<Matte<Addition> >(x, y, width, height, src); break;
        case 4: CopySprite16<Matte<Subtraction> >(x, y, width, height, src); break;
        case 5: CopySprite16<Matte<VarAddition> >(x, y, width, height, src); break;
        case 6: CopySprite16<Matte<VarSubtraction> >(x, y, width, height, src); break;
        }
        return;
    }

    // else 8bit blit

    int xl, yl, xs, ys;
    int a;
    
    if (screen==NULL) return;
    
    cpubyte = RENDER;
    xl = width;
    yl = height;
    xs = ys = 0;
    if (x>clip.right || y>clip.bottom || x+xl<clip.left || y+yl<clip.top)
        return;
    
    if (x+xl-1 > clip.right ) xl = clip.right -x+1;
    if (y+yl-1 > clip.bottom) yl = clip.bottom-y+1;
    if (x<clip.left) { xs = clip.left-x;         xl-=xs;   x = clip.left; }
    if (y<clip.top ) { ys = clip.top -y;         yl-=ys;   y = clip.top ; }
    
        u8 *s8, *d8;
        s8 = src+ys*width+xs;
        d8 = screen+y*scrx+x;
        
    for (; yl; yl--)
    {
        for (a = 0; a<xl; a++)
            if (s8[a])
                d8[a]=lucentlut8[d8[a]|(s8[a]<<8)];
            s8+=width;
            d8+=scrx;
    }

    cpubyte = ETC;
}

void GrDriver::ScaleSpriteLucent(int x, int y, int iwidth, int iheight, int dwidth, int dheight, u8* src, int lucent)
{
    if (bpp == 2)
    {
        switch (lucent)
        {
        default:
        case 0: ScaleSprite16<Opaque>(x, y, iwidth, iheight, dwidth, dheight, src); break;
        case 1: ScaleSprite16<AlphaHalf>(x, y, iwidth, iheight, dwidth, dheight, src); break;
        case 2: ScaleSprite16<Alpha>(x, y, iwidth, iheight, dwidth, dheight, src); break;
        case 3: ScaleSprite16<Addition>(x, y, iwidth, iheight, dwidth, dheight, src); break;
        case 4: ScaleSprite16<Subtraction>(x, y, iwidth, iheight, dwidth, dheight, src); break;
        case 5: ScaleSprite16<VarAddition>(x, y, iwidth, iheight, dwidth, dheight, src); break;
        case 6: ScaleSprite16<VarSubtraction>(x, y, iwidth, iheight, dwidth, dheight, src); break;
        }
        return;
    }

    // 8bpp shit here

    int i, j;
    int xerr, yerr;
    int xerr_start, yerr_start;
    int xadj, yadj;
    int xl, yl, xs, ys;
    int c;
    
    cpubyte = RENDER;
    
    if (dwidth < 1 || dheight < 1)
        return;
    
    xl = dwidth;
    yl = dheight;
    xs = ys = 0;
    if (x > clip.right || y > clip.bottom
        || x + xl < clip.left || y + yl < clip.top)
        return;
    
    if (x+xl-1 > clip.right)   xl = clip.right - x + 1;
    if (y+yl-1 > clip.bottom)  yl = clip.bottom - y + 1;
    
    if (x < clip.left) { xs = clip.left - x; xl -= xs; x = clip.left; }
    if (y < clip.top) { ys = clip.top - y; yl -= ys; y = clip.top; }
    
    xadj = (iwidth << 16)/dwidth;
    yadj = (iheight << 16)/dheight;
    xerr_start = xadj * xs;
    yerr_start = yadj * ys;
    
    yerr = yerr_start & 0xffff;
    
    u8 *s8, *d8;
    
    s8 = src+(ys*iwidth);
    d8 = screen+y*scrx+x;
    
    for (i = 0; i < yl; i += 1)
    {
        xerr = xerr_start;
        for (j = 0; j < xl; j += 1)
        {
            c = s8[(xerr >> 16)];
            d8[j]=lucentlut8[d8[j] | (c << 8)];
            xerr += xadj;
        }
        d8   += scrx;
        yerr += yadj;
        s8   += (yerr >> 16)*iwidth;
        yerr &= 0xffff;
    }
}

void GrDriver::TScaleSpriteLucent(int x, int y, int iwidth, int iheight, int dwidth, int dheight, u8* src, int lucent)
{
    if (bpp == 2)
    {
        switch (lucent)
        {
        default:
        case 0: ScaleSprite16<Matte<Opaque> >(x, y, iwidth, iheight, dwidth, dheight, src); break;
        case 1: ScaleSprite16<Matte<AlphaHalf> >(x, y, iwidth, iheight, dwidth, dheight, src); break;
        case 2: ScaleSprite16<Matte<Alpha> >(x, y, iwidth, iheight, dwidth, dheight, src); break;
        case 3: ScaleSprite16<Matte<Addition> >(x, y, iwidth, iheight, dwidth, dheight, src); break;
        case 4: ScaleSprite16<Matte<Subtraction> >(x, y, iwidth, iheight, dwidth, dheight, src); break;
        case 5: ScaleSprite16<Matte<VarAddition> >(x, y, iwidth, iheight, dwidth, dheight, src); break;
        case 6: ScaleSprite16<Matte<VarSubtraction> >(x, y, iwidth, iheight, dwidth, dheight, src); break;
        }
        return;
    }

    int i, j;
    int xerr, yerr;
    int xerr_start, yerr_start;
    int xadj, yadj;
    int xl, yl, xs, ys;
    int c;
    
    cpubyte = RENDER;
    
    if (dwidth < 1 || dheight < 1)
        return;
    
    xl = dwidth;
    yl = dheight;
    xs = ys = 0;
    if (x > clip.right || y > clip.bottom
        || x + xl < clip.left || y + yl < clip.top)
        return;
    
    if (x+xl-1 > clip.right)   xl = clip.right - x + 1;
    if (y+yl-1 > clip.bottom)  yl = clip.bottom - y + 1;
    
    if (x < clip.left) { xs = clip.left - x; xl -= xs; x = clip.left; }
    if (y < clip.top) { ys = clip.top - y; yl -= ys; y = clip.top; }
    
    xadj = (iwidth << 16)/dwidth;
    yadj = (iheight << 16)/dheight;
    xerr_start = xadj * xs;
    yerr_start = yadj * ys;
    
    yerr = yerr_start & 0xffff;
    
    u8 *s8, *d8;
    
    s8 = src+(ys*iwidth);
    d8 = screen+y*scrx+x;
    
    for (i = 0; i < yl; i += 1)
    {
        xerr = xerr_start;
        for (j = 0; j < xl; j += 1)
        {
            c = s8[(xerr >> 16)];
            if (c)
                d8[j] = lucentlut8[d8[j] | (c << 8)];
            xerr += xadj;
        }
        d8   += scrx;
        yerr += yadj;
        s8   += (yerr >> 16)*iwidth;
        yerr &= 0xffff;
    }
}

void GrDriver::RotScaleLucent(int posx, int posy, int width, int height, float angle, float scale, u8* src, int lucent)
{
    // new! shamelessly ripped off from alias.zip
    // except the atan2 stuff which i had to make up myself AEN so there :p
    if (bpp == 2)
    {
        switch (lucent)
        {
        default:
        case 0: RotScaleSprite16<Matte<Opaque> >(posx, posy, width, height, angle, scale, src); break;
        case 1: RotScaleSprite16<Matte<AlphaHalf> >(posx, posy, width, height, angle, scale, src); break;
        case 2: RotScaleSprite16<Matte<Alpha> >(posx, posy, width, height, angle, scale, src); break;
        case 3: RotScaleSprite16<Matte<Addition> >(posx, posy, width, height, angle, scale, src); break;
        case 4: RotScaleSprite16<Matte<Subtraction> >(posx, posy, width, height, angle, scale, src); break;
        case 5: RotScaleSprite16<Matte<VarAddition> >(posx, posy, width, height, angle, scale, src); break;
        case 6: RotScaleSprite16<Matte<VarSubtraction> >(posx, posy, width, height, angle, scale, src); break;
        }
        return;
    }
    
    int xs, ys, xl, yl;
    int sinas, cosas, xc, yc, srcx, srcy, x, y, tempx, tempy, T_WIDTH_CENTER, T_HEIGHT_CENTER, W_WIDTH_CENTER, W_HEIGHT_CENTER, W_HEIGHT, W_WIDTH;
    u16 pt;
    float ft;
    
    ft=(float)atan2((float)width, (float)height);
    
    T_WIDTH_CENTER = width>>1;
    T_HEIGHT_CENTER = height>>1;
    W_WIDTH=(int)((float)width/scale*sin(ft) + (float)height/scale*cos(ft));
    W_HEIGHT = W_WIDTH;
    W_HEIGHT_CENTER = W_HEIGHT>>1;
    W_WIDTH_CENTER = W_HEIGHT_CENTER; //W_WIDTH/2;
    
    sinas=(int)(sin(-angle)*65536*scale);
    cosas=(int)(cos(-angle)*65536*scale);
    
    xc = T_WIDTH_CENTER*65536 - (W_HEIGHT_CENTER*(cosas+sinas));
    yc = T_HEIGHT_CENTER*65536 - (W_WIDTH_CENTER*(cosas-sinas));
    posx-=W_WIDTH_CENTER;
    posy-=W_HEIGHT_CENTER;
    
    // clipping
    if (W_WIDTH<2 || W_HEIGHT<2) return;
    xl = W_WIDTH;
    yl = W_HEIGHT;
    xs = ys = 0;
    if (posx>clip.right || posy>clip.bottom || posx+xl<clip.left || posy+yl<clip.top)
        return;
    if (posx+xl-1 > clip.right) xl = clip.right-posx+1;
    if (posy+yl-1 > clip.bottom) yl = clip.bottom-posy+1;
    if (posx<clip.left)
    {
        xs = clip.left-posx;
        xl-=xs;
        posx = clip.left;
        
        xc+=cosas*xs; // woo!
        yc-=sinas*xs;
    }
    if (posy<clip.top)
    {
        ys = clip.top-posy;
        yl-=ys;
        posy = clip.top;
        
        xc+=sinas*ys; // woo!
        yc+=cosas*ys;
    }
    
    
    u8 *s8, *d8;
    d8 = screen+posx+posy*scrx;
    s8 = src;
    for (y = 0; y<yl; y++)
    {
        srcx = xc;
        srcy = yc;
        
        for (x = 0; x<xl; x++)
        {
            tempx=(srcx>>16);
            tempy=(srcy>>16);
            
            if (tempx>=0 && tempx<width && tempy>=0 && tempy<height)
            {
                pt = s8[tempx+tempy*width];
                if (pt)
                    d8[x]=lucentlut8[(pt<<8)|d8[x]];
            }
            
            srcx+=cosas;
            srcy-=sinas;
        }
        
        d8+=scrx;
        
        xc+=sinas;
        yc+=cosas;
    }
}

void GrDriver::BlitStipple(int x, int y, int colour)
{
    int xl, yl, xs, ys;
    int ax, ay;
    
    xl = 16;
    yl = 16;
    xs = ys = 0;
    if (x>clip.right || y>clip.bottom || x+xl<clip.left || y+yl<clip.top)
        return;
    if (x+xl-1 > clip.right)  xl = clip.right-x+1;
    if (y+yl-1 > clip.bottom) yl = clip.bottom-y+1;
    if (x<clip.left) { xl+=x-clip.left;   x = clip.left; }
    if (y<clip.top ) { yl+=y-clip.top;   y = clip.top ; }
    
    if (bpp==1)
    {
        u8* d8;
        d8 = screen+y*scrx+x;
        for (ay = 0; ay<yl; ay++)
        {
            for (ax = 0; ax<xl; ax++)
                if ((ax+ay)&1) d8[ax]=colour;
                d8+=scrx;
        }
    }
    else
    {
        u16* d16;
        d16=(u16*)(screen)+y*scrx+x;
        for (ay = 0; ay<yl; ay++)
        {
            ax = xl;
            while (ax--)
                SetPixelLucent(d16++, colour, 1);
            d16+=scrx-xl;
        }
    }
}

void GrDriver::WrapBlitLucent(int x, int y, int width, int height, u8* src, int lucent)
{
    if (bpp == 2)
    {
        switch (lucent)
        {
        default:
        case 0: WrapSprite16<Opaque>(x, y, width, height, src); break;
        case 1: WrapSprite16<AlphaHalf>(x, y, width, height, src); break;
        case 2: WrapSprite16<Alpha>(x, y, width, height, src); break;
        case 3: WrapSprite16<Addition>(x, y, width, height, src); break;
        case 4: WrapSprite16<Subtraction>(x, y, width, height, src); break;
        case 5: WrapSprite16<VarAddition>(x, y, width, height, src); break;
        case 6: WrapSprite16<VarSubtraction>(x, y, width, height, src); break;
        }
        return;
    }

    int cur_x, sign_y;
    
    if (width < 1 || height < 1)
        return;
    
    x %= width, y %= height;
    sign_y = 0 - y;
    
    for (; sign_y < scry; sign_y += height)
    {
        for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
            CopySpriteLucent(cur_x, sign_y, width, height, src, lucent);
    }
}

void GrDriver::TWrapBlitLucent(int x, int y, int width, int height, u8* src, int lucent)
{
    if (bpp == 2)
    {
        switch (lucent)
        {
        default:
        case 0: WrapSprite16<Matte<Opaque> >(x, y, width, height, src); break;
        case 1: WrapSprite16<Matte<AlphaHalf> >(x, y, width, height, src); break;
        case 2: WrapSprite16<Matte<Alpha> >(x, y, width, height, src); break;
        case 3: WrapSprite16<Matte<Addition> >(x, y, width, height, src); break;
        case 4: WrapSprite16<Matte<Subtraction> >(x, y, width, height, src); break;
        case 5: WrapSprite16<Matte<VarAddition> >(x, y, width, height, src); break;
        case 6: WrapSprite16<Matte<VarSubtraction> >(x, y, width, height, src); break;
        }
        return;
    }

    int cur_x, sign_y;
    
    if (width < 1 || height < 1)
        return;
    
    x %= width, y %= height;
    sign_y = 0 - y;
    
    for (; sign_y < scry; sign_y += height)
    {
        for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
            TCopySpriteLucent(cur_x, sign_y, width, height, src, lucent);
    }
}


// primitives

void GrDriver::SetPixel(int x, int y, int colour, int lucent)
{
    int ofs;
    ofs = y*scrx+x;
    
    if (x<clip.left || y<clip.top || x>clip.right || y>clip.bottom) return;
    
    if (bpp==1)
    {
        u8* p = screen+ofs;
        if (lucent)
            *p = lucentlut8[colour || *p<<8];
        else
            *p = colour;
    }
    else 
    {
        u16* p=(u16*)screen+ofs;
        if (lucent)
            SetPixelLucent(p, colour, lucent);
        else
            *p = colour;
    }
}

int  GrDriver::GetPixel(int x, int y)
{
    if (x<clip.left || y<clip.top || x>clip.right || y>clip.bottom) return 0;
    
    if (bpp==1)
    {
        u8 *c;
        c = screen+y*scrx+x;
        return *c;
    }
    else
    {  
        u16 *w;
        w=(u16*)screen+y*scrx+x;
        return *w;
    }
    
}

void GrDriver::HLine(int x, int y, int x2, int colour, int lucent)
{
    int xl;
    // range checking
    if (x>x2)
    {
        int a = x2;
        x2 = x;
        x = a;
    } // swap 'em
    
    if (x<clip.left) x = clip.left;
    if (x2>clip.right) x2 = clip.right;
    
    // Is the line even onscreen?
    if (y<clip.top)  return;
    if (y>clip.bottom) return;
    if (x>clip.right) return;
    if (x2<clip.left) return;
    
    if (bpp==1)
    {
        u8 *d8;
        d8 = screen+(y*scrx+x);
        xl = x2-x+1;
        if (lucent)
            while (xl--)
                *d8 = lucentlut8[*d8++|(colour<<8)];
            else
                memset(d8, (char)colour, xl); // woo. hard
    }
    else
    {
        u16* d16;
        d16=(u16*)(screen)+y*scrx+x;
        xl = x2-x+1;
        if (lucent)
            while (xl--)
                SetPixelLucent(d16++, colour, lucent);
            else
                while (xl--)
                    *d16++=(u16)colour; // woo. also hard.
    }
}

void GrDriver::VLine(int x, int y, int y2, int colour, int lucent)
{
    if (y>y2)
    {
        int a = y2;
        y2 = y;
        y = a;
    } // swap 'em
    
    if (y<clip.top) y = clip.top;
    if (y2>clip.bottom) y2 = clip.bottom;
    
    // Is the line even onscreen?
    if (y2<clip.top)  return;
    if (y>clip.bottom) return;
    if (x>clip.right) return;
    if (x<clip.left) return;
    
    if (bpp==1)
    {
        u8 *d8;
        d8 = screen+(y*scrx+x);
        int yl = y2-y+1;
        if (lucent)
            while (yl--)
            {
                *d8 = lucentlut8[*d8|(colour<<8)];
                d8+=scrx;
            }
            else
                while (yl--)
                {
                    *d8=(u8)colour;
                    d8+=scrx;
                }
    }
    else
    {
        u16* d16;
        d16=(u16*)(screen)+y*scrx+x;
        int yl = y2-y+1;
        if (lucent)
            while (yl--)
            {
                SetPixelLucent(d16, colour, lucent);
                d16+=scrx;
            }
            else
                while (yl--)
                {
                    *d16=(u16)colour;
                    d16+=scrx;
                }
    }
}

void swap(int& a, int& b)
{
    int c;
    c = a;
    a = b;
    b = c;
}

void GrDriver::Line(int x1, int y1, int x2, int y2, int colour, int lucent)
{
    short i, xc, yc, er, n, m, xi, yi, xcxi, ycyi, xcyi;
    unsigned dcy, dcx;
    
    cpubyte = RENDER;
    // check to see if the line is completly clipped off
    if ((x1<clip.left && x2<clip.left) || (x1>clip.right && x2>clip.right)
        || (y1<clip.top && y2<clip.top) || (y1>clip.bottom && y2>clip.bottom))
    {
        cpubyte = ETC;
        return;
    }
    
    if (x1>x2)
    {
        swap(x1, x2);
        swap(y1, y2);
    }
    
    // clip the left side
    if (x1<clip.left)
    { int myy=(y2-y1);
    int mxx=(x2-x1), b;
    if (!mxx)
    {
        cpubyte = ETC;
        return;
    }
    if (myy)
    {
        b = y1-(y2-y1)*x1/mxx;
        y1 = myy*clip.left/mxx+b;
        x1 = clip.left;
    }
    else x1 = clip.left;
    }
    
    // clip the right side
    if (x2>clip.right)
    { int myy=(y2-y1);
    int mxx=(x2-x1), b;
    if (!mxx)
    {
        cpubyte = ETC;
        return;
    }
    if (myy)
    {
        b = y1-(y2-y1)*x1/mxx;
        y2 = myy*clip.right/mxx+b;
        x2 = clip.right;
    }
    else x2 = clip.right;
    }
    
    if (y1>y2)
    {
        swap(x1, x2);
        swap(y1, y2);
    }
    
    // clip the bottom
    if (y2>clip.bottom)
    { int mxx=(x2-x1);
    int myy=(y2-y1), b;
    if (!myy)
    {
        cpubyte = ETC;
        return;
    }
    if (mxx)
    {
        b = y1-(y2-y1)*x1/mxx;
        x2=(clip.bottom-b)*mxx/myy;
        y2 = clip.bottom;
    }
    else y2 = clip.bottom;
    }
    
    // clip the top
    if (y1<clip.top)
    { int mxx=(x2-x1);
    int myy=(y2-y1), b;
    if (!myy)
    {
        cpubyte = ETC;
        return;
    }
    if (mxx)
    {
        b = y1-(y2-y1)*x1/mxx;
        x1=(clip.top-b)*mxx/myy;
        y1 = clip.top;
    }
    else y1 = clip.top;
    }
    
    // ???
    // see if it got cliped into the box, out out
    if (x1<clip.left || x2<clip.left || x1>clip.right || x2>clip.right || y1<clip.top || y2 <clip.top || y1>clip.bottom || y2>clip.bottom)
    {
        cpubyte = ETC;
        return;
    }
    
    if (x1>x2)
    { xc = x2; xi = x1; }
    else { xi = x2; xc = x1; }
    
    // assume y1<=y2 from above swap operation
    yi = y2; yc = y1;
    
    dcx = x1; dcy = y1;
    xc=(x2-x1); yc=(y2-y1);
    if (xc<0) xi=-1; else xi = 1;
    if (yc<0) yi=-1; else yi = 1;
    n = abs(xc); m = abs(yc);
    ycyi = abs(2*yc*xi);
    er = 0;
    
    if (bpp==1)
    {
        if (n>m)
        {
            xcxi = abs(2*xc*xi);
            if (lucent)
                for (i = 0;i<=n;i++)
                {
                    screen[(dcy*scrx)+dcx]=lucentlut8[colour|screen[(dcy*scrx)+dcx]<<8];
                    if (er>0)
                    { dcy+=yi;
                    er-=xcxi;
                    }
                    er+=ycyi;
                    dcx+=xi;
                }        
                else
                    for (i = 0;i<=n;i++)
                    {
                        screen[(dcy*scrx)+dcx]=colour;
                        if (er>0)
                        { dcy+=yi;
                        er-=xcxi;
                        }
                        er+=ycyi;
                        dcx+=xi;
                    }
        }
        else
        {
            xcyi = abs(2*xc*yi);
            if (lucent)
                for (i = 0;i<=m;i++)
                {
                    screen[(dcy*scrx)+dcx]=lucentlut8[colour|screen[(dcy*scrx)+dcx]<<8];
                    if (er>0)
                    { dcx+=xi;
                    er-=ycyi;
                    }
                    er+=xcyi;
                    dcy+=yi;
                }
                else
                    for (i = 0;i<=m;i++)
                    {
                        screen[(dcy*scrx)+dcx]=colour;
                        if (er>0)
                        { dcx+=xi;
                        er-=ycyi;
                        }
                        er+=xcyi;
                        dcy+=yi;
                    }
        }
    }
    else
    {
        u16* s16=(u16*) screen;
        if (n>m)
        {
            xcxi = abs(2*xc*xi);
            if (lucent)
                for (i = 0;i<=n;i++)
                {
                    //             s16[(dcy*scrx)+dcx]=colour;
                    SetPixelLucent(&s16[(dcy*scrx)+dcx], colour, lucent);
                    if (er>0)
                    { dcy+=yi;
                    er-=xcxi;
                    }
                    er+=ycyi;
                    dcx+=xi;
                }
                else
                    for (i = 0;i<=n;i++)
                    {
                        s16[(dcy*scrx)+dcx]=colour;
                        if (er>0)
                        { dcy+=yi;
                        er-=xcxi;
                        }
                        er+=ycyi;
                        dcx+=xi;
                    }
        }
        else
        {
            xcyi = abs(2*xc*yi);
            for (i = 0;i<=m;i++)
            {
                s16[(dcy*scrx)+dcx]=colour;
                if (er>0)
                { dcx+=xi;
                er-=ycyi;
                }
                er+=xcyi;
                dcy+=yi;
            }
        }
    }
    cpubyte = ETC;
    return;
}

void GrDriver::Rect(int x1, int y1, int x2, int y2, int colour, int lucent)
{
    int a;
    if (x1>x2) { a = x1; x1 = x2; x2 = a; } // swap 'em
    if (y1>y2) { a = y1; y1 = y2; y2 = a; }
    
    // hline and vline do their own range checking anyway.
    VLine(x1, y1, y2, colour, lucent);
    VLine(x2, y1, y2, colour, lucent);
    HLine(x1, y1, x2, colour, lucent);
    HLine(x1, y2, x2, colour, lucent);
}

void GrDriver::RectFill(int x1, int y1, int x2, int y2, int colour, int lucent)
{
    int a;
    if (x1>x2) { a = x1; x1 = x2; x2 = a; } // swap 'em
    if (y1>y2) { a = y1; y1 = y2; y2 = a; }
    
    // range checking
    if (x1<clip.left) x1 = clip.left;
    if (x2>clip.right) x2 = clip.right;
    if (y1<clip.top) y1 = clip.top;
    if (y2>clip.bottom) y2 = clip.bottom;
    
    
    for (a = y1; a<=y2; a++)
        HLine(x1, a, x2, colour, lucent);
}

void GrDriver::Circle(int x, int y, int radius, int colour, int lucent)
{
    int cx = 0;
    int cy = radius;
    int df = 1 - radius;
    int d_e = 3;
    int d_se = -2*radius + 5;
    
    cpubyte = RENDER;
    
    do
    {
        SetPixel(x + cx, y + cy, colour, lucent);
        if (cx) SetPixel(x - cx, y + cy, colour, lucent);
        if (cy) SetPixel(x + cx, y - cy, colour, lucent);
        if (cx && cy) SetPixel(x - cx, y - cy, colour, lucent);
        
        if (cx != cy)
        {
            SetPixel(x + cy, y + cx, colour, lucent);
            if (cx) SetPixel(x + cy, y - cx, colour, lucent);
            if (cy) SetPixel(x - cy, y + cx, colour, lucent);
            if (cx && cy) SetPixel(x - cy, y - cx, colour, lucent);
        }
        
        if (df < 0)
        {
            df += d_e;
            d_e += 2;
            d_se += 2;
        }
        else
        {
            df += d_se;
            d_e += 2;
            d_se += 4;
            cy -= 1;
        }
        cx += 1;
    }
    while (cx <= cy);
    
    cpubyte = ETC;
}




void GrDriver::CircleFill(int x, int y, int radius, int colour, int lucent)
{
    int cx = 0;
    int cy = radius;
    int df = 1 - radius;
    int d_e = 3;
    int d_se = -2*radius + 5;
    
    cpubyte = RENDER;
    
    do
    {
        HLine(x - cy, y - cx, x + cy, colour, lucent);
        if (cx) HLine(x - cy, y + cx, x + cy, colour, lucent);
        
        if (df < 0)
        {
            df += d_e;
            d_e += 2;
            d_se += 2;
        }
        else
        {
            if (cx != cy)
            {
                HLine(x - cx, y - cy, x + cx, colour, lucent);
                if (cy) HLine(x - cx, y + cy, x + cx, colour, lucent);
            }
            df += d_se;
            d_e += 2;
            d_se += 4;
            cy -= 1;
        }
        cx += 1;
    }
    while (cx <= cy);
    
    cpubyte = ETC;
}

void GrDriver::FlatPoly(int x1, int y1, int x2, int y2, int x3, int y3, int colour)
{
    int xstep, xstep2;
    int xval, xval2;
    int yon;
    int swaptemp;
    
    if (y1 > y3)
    {
        swaptemp = x1; x1 = x3; x3 = swaptemp;
        swaptemp = y1; y1 = y3; y3 = swaptemp;
    }
    if (y2 > y3)
    {
        swaptemp = x2; x2 = x3; x3 = swaptemp;
        swaptemp = y2; y2 = y3; y3 = swaptemp;
    }
    if (y1 > y2)
    {
        swaptemp = x1; x1 = x2; x2 = swaptemp;
        swaptemp = y1; y1 = y2; y2 = swaptemp;
    }
    
    xstep2=((x3-x1) << 16) / (y3-y1);
    xval2 = x1 << 16;
    
    if (y1 != y2)
    {
        xstep = ((x2-x1) << 16) / (y2-y1);
        xval = x1 << 16;
        for (yon = y1;yon < y2; yon++)
        {
            if ((yon > clip.top) && (yon < clip.bottom))
            {
                HLine(xval>>16, yon, xval2>>16, colour, 0);
            }
            xval+=xstep;
            xval2+=xstep2;
        }
    }
    if (y2 != y3)
    {
        xstep = ((x3-x2) << 16) / (y3-y2);
        xval = x2 << 16;
        for (yon = y2;yon < y3; yon++)
        {
            if ((yon > clip.top) && (yon < clip.bottom))
            {
                HLine(xval>>16, yon, xval2>>16, colour, 0);
            }
            xval+=xstep;
            xval2+=xstep2;
        }
    }
}

inline void GrDriver::tmaphline(int x1, int x2, int y, int tx1, int tx2, int ty1, int ty2, int texw, int texh, u8* image)
{
    int i;
    int txstep, txval;
    int tystep, tyval;
    
    if (x1 != x2)
    {
        if (x2 < x1)
        {
            i = x1; x1 = x2; x2 = i;
            i = tx1; tx1 = tx2; tx2 = i;
            i = ty1; ty1 = ty2; ty2 = i;
        }
        if ((x1 > scrx) || (x2 < 0)) return;
        txstep=((tx2-tx1)<<16)/(x2-x1);
        tystep=((ty2-ty1)<<16)/(x2-x1);
        txval = tx1<<16;
        tyval = ty1<<16;
        u16* s16=(u16*)screen;
        u16* d16=(u16*)image;
        
        if (bpp==1)
            for (i = x1;i<x2;i++)
            {
                screen[y*scrx+i] = image[(tyval>>16)*texw+(txval>>16)];
                txval+=txstep;
                tyval+=tystep;
            }
            else
                for (i = x1;i<x2;i++)
                {
                    s16[y*scrx+i] = d16[(tyval>>16)*texw+(txval>>16)];
                    txval+=txstep;
                    tyval+=tystep;
                }
    }
}

void GrDriver::TMapPoly(int x1, int y1, int x2, int y2, int x3, int y3,
                        int tx1, int ty1, int tx2, int ty2, int tx3, int ty3,
                        int tw, int th, u8 *img)
{
    int xstep, xstep2;
    int xval, xval2;
    int txstep, txstep2;
    int tystep, tystep2;
    int txval, txval2;
    int tyval, tyval2;
    int yon;
    int swaptemp;
    
    u8* image;
    int texw, texh;
    
    image = img; texw = tw; texh = th;
    if (y1 > y3)
    {
        swaptemp = x1; x1 = x3; x3 = swaptemp;
        swaptemp = y1; y1 = y3; y3 = swaptemp;
        swaptemp = tx1; tx1 = tx3; tx3 = swaptemp;
        swaptemp = ty1; ty1 = ty3; ty3 = swaptemp;
    }
    if (y2 > y3)
    {
        swaptemp = x2; x2 = x3; x3 = swaptemp;
        swaptemp = y2; y2 = y3; y3 = swaptemp;
        swaptemp = tx2; tx2 = tx3; tx3 = swaptemp;
        swaptemp = ty2; ty2 = ty3; ty3 = swaptemp;
    }
    if (y1 > y2)
    {
        swaptemp = x1; x1 = x2; x2 = swaptemp;
        swaptemp = y1; y1 = y2; y2 = swaptemp;
        swaptemp = tx1; tx1 = tx2; tx2 = swaptemp;
        swaptemp = ty1; ty1 = ty2; ty2 = swaptemp;
    }
    xstep2=((x3-x1) << 16) / (y3-y1);
    xval2 = x1 << 16;
    txstep2=((tx3-tx1) << 16) / (y3-y1);
    txval2 = tx1 << 16;
    tystep2=((ty3-ty1) << 16) / (y3-y1);
    tyval2 = ty1 << 16;
    
    if (y1 != y2)
    {
        xstep = ((x2-x1) << 16) / (y2-y1);
        xval = x1 << 16;
        txstep = ((tx2-tx1) << 16) / (y2-y1);
        txval = tx1 << 16;
        tystep = ((ty2-ty1) << 16) / (y2-y1);
        tyval = ty1 << 16;
        
        for (yon = y1;yon < y2; yon++)
        {
            if ((yon > clip.top) && (yon < clip.bottom))
                tmaphline(xval>>16, xval2>>16, yon, txval>>16, txval2>>16,
                tyval>>16, tyval2>>16,
                texw, texh, image);
            
            xval+=xstep;
            xval2+=xstep2;
            txval+=txstep;
            txval2+=txstep2;
            tyval+=tystep;
            tyval2+=tystep2;
        }
    }
    if (y2 != y3)
    {
        xstep = ((x3-x2) << 16) / (y3-y2);
        xval = x2 << 16;
        txstep = ((tx3-tx2) << 16) / (y3-y2);
        txval = tx2 << 16;
        tystep = ((ty3-ty2) << 16) / (y3-y2);
        tyval = ty2 << 16;
        
        for (yon = y2;yon < y3; yon++)
        {
            if ((yon > clip.top) && (yon < clip.bottom))
            {
                tmaphline(xval>>16, xval2>>16, yon, txval>>16, txval2>>16,
                    tyval>>16, tyval2>>16,
                    texw, texh, image);
            }
            xval+=xstep;
            xval2+=xstep2;
            txval+=txstep;
            txval2+=txstep2;
            tyval+=tystep;
            tyval2+=tystep2;
        }
    }
}


void GrDriver::Mask(u8* src, u8* mask, int width, int height, u8* dest)
{
    int i = width*height;
    if (bpp==2)
    {
        u16* s16=(u16*)src;
        u16* m16=(u16*)mask;
        u16* d16=(u16*)dest; // bleh, makes my life easier
        
        while (i--)
        {
            *d16=(*s16&*m16);
            d16++; s16++; m16++;
        }
        // hmm.. simple enough. ;)
    }
    else
    {
        while (i--)
        {
            *dest=(*src&*mask);
            dest++; mask++; src++;
        }
    }
}

void GrDriver::Silhouette(int width, int height, u8* src, u8* dest, int colour)
{
    width*=height;
    if (bpp==1)
    {
        while (width--)
        {
            if (*src) 
                *dest = colour;
            src++; dest++;
        }
    }
    else 
    {
        u16* s16=(u16*)src;
        u16* d16=(u16*)dest;
        while (width--)
        {
            if (*s16!=trans_mask)
                *d16 = colour;
            s16++; d16++;
        }
    }
}

void GrDriver::ChangeAll(int width, int height, u8* src, int srccolour, int destcolour)
{
    width*=height;
    if (bpp==1)
    {
        while (width--)
        {
            if (*src==srccolour)
                *src = destcolour;
            src++;
        }
    }
    else
    {
        u16* s16=(u16*)src;
        while (width--)
        {
            if (*s16==srccolour)
                *s16 = destcolour;
            s16++;
        }       
    }
}

// Pixel/palette crap

void GrDriver::GetPixelFormat(void)
{
    SDL_PixelFormat& fmt = *mainSurface->format;
    bpos = fmt.Bshift;  bsize = 8 - fmt.Bloss;
    gpos = fmt.Gshift;  gsize = 8 - fmt.Gloss;
    rpos = fmt.Rshift;  rsize = 8 - fmt.Rloss;
    
    lucentmask=~((1<<rpos)|(1<<gpos)|(1<<bpos));
}

// converts an 8 bit palettized pixel to the current bit depth
unsigned int GrDriver::Conv8(int c)
{
    return Conv8(gamepal, c);
}

unsigned int GrDriver::Conv8(u8* pal, int c)
{
    if (bpp==1) return c; // we'll assume it's the same palette for now.
    
    if (!c)
        return trans_mask;
    else
    {
        int i = c*3;
        u8 r = pal[i++];
        u8 g = pal[i++];
        u8 b = pal[i++];
        
        return PackPixel(r, g, b);
    }
}

unsigned int GrDriver::Conv16(int c)
// converts a 5:6:5 hicolour pixel to the current bit depth
{
    int r, g, b;
    
    b = c&31;
    g=(c>>5)&63;
    r=(c>>11)&31;
    // b<<=1; r<<=1;
    
    return PackPixel(r<<3, g<<2, b<<3);
}

unsigned int GrDriver::PackPixel(int r, int g, int b)
{
    if (rsize<8)  r>>=8-rsize;
    if (gsize<8)  g>>=8-gsize;
    if (bsize<8)  b>>=8-bsize;
    
    return (r<<rpos) | (g<<gpos) | (b<<bpos);
    // TODO: add closest-matching-colour function for 8bit modes.
}

void GrDriver::UnPackPixel(int c, int& r, int& g, int& b)
{
    r = c>>rpos;
    g = c>>gpos;
    b = c>>bpos;
    
    if (rsize<8)  r=(r<<(8-rsize))&255;
    if (gsize<8)  g=(g<<(8-gsize))&255;
    if (bsize<8)  b=(b<<(8-bsize))&255;
}

int GrDriver::SetPalette(u8* p) // p is a char[768]
{
    //static bool bBeenhere = false;
    //bool bBeenhere = false;
    
    if (!p) return 0; // ...
    
    memcpy(gamepal, p, 768);
    
    
    if (bpp==1)                             // need to adjust the hardware palette if we're in a palettized graphics mode
    {    
        if (p!=pal)
        {
            memcpy(pal, p, 768);                                // we'll keep the pal member updated while we're at it.
            UpdatePalette(pal);
        }
    }
    return 1;
}

// This does nothing more than send the palette data to SDL
void GrDriver::UpdatePalette(u8* pal) // char[768]
{
    SDL_Color sdlpal[256];
    
    // copy the pal array into ddp
    for (int i = 0; i<256; i++)
    {
        sdlpal[i].r = pal[i*3  ];
        sdlpal[i].g = pal[i*3+1];
        sdlpal[i].b = pal[i*3+2];
    }
    
    int result = SDL_SetColors(mainSurface, sdlpal, 0, 256);
    if (result!=1)
        Log::Write("uh oh, palette problem");            
}

int GrDriver::GetPalette(u8* p) // p is a char[768]
{
    if (bpp>1) return 1;
    
    memcpy(p, pal, 768);
    return 1;
}

int GrDriver::InitLucentLUT(u8* data)
// this just copies the data into lucentlut8 so the lucent stuff can work in 8bit mode.
{
    if (!lucentlut8)
        lucentlut8 = new char[256*256];
    memcpy(lucentlut8.get(), data, 256*256);
    return 1; // bleh, simple
}

void GrDriver::CalcLucentLUT(int lucency)
{
    int biggest = max(rsize, max(bsize, gsize));
    /*int biggest;
    
    biggest = rsize;
    biggest = biggest < gsize ? gsize : biggest;
    biggest = biggest < bsize ? bsize : biggest;*/
    
    if (!lucentlut16)
        lucentlut16 = new u16[65535];
    
    int i, j;
    
    for (i = 0; i<256; i++)
        for (j = 0; j<256; j++)
        {
            lucentlut16[i*256+j]=(i*lucency+j*(255-lucency))/256;
            //i*32+j == (i*tlevel+j*(255-tlevel))/256;
        }
        
}

void GrDriver::SetClipRect(struct SRect& newrect)
{
    clip = newrect;
}

void GrDriver::SetRenderDest(int x, int y, u8* dest)
{
    scrx = x; scry = y;
    clip.top = clip.left = 0;
    clip.right = x - 1; clip.bottom = y - 1;
    screen = dest;
}

void GrDriver::RestoreRenderSettings()
{
    scrx = xres; scry = yres;
    clip.top = clip.left = 0;
    clip.right = xres - 1; clip.bottom = yres - 1;
    screen = trueScreen;
}

inline u8 GrDriver::morph_step(int S, int D, int mix, int light)
{
    return (mix*(S - D) + (100 * D)) / 100 * light / 63;
}

void GrDriver::PaletteMorph(int mr, int mg, int mb, int percent, int intensity)
{
    int rgb[3], n;
    u8 pmorph_palette[3*256];
    
    int i;
    int wr, wg, wb;
    int r, g, b;
    
    if (bpp==2)
    {
        if (percent && intensity==63)
        {
            morphlut = 0;
            return;
        }
        if (!morphlut)  
            morphlut = new u16[65536]; // YES, I like look-up tables, thank you very much. --tSB
        for (i = 0; i<65535; i++)
        {
            UnPackPixel(i, r, g, b);
            
            wr = morph_step(r, mr, percent, intensity);
            wg = morph_step(g, mg, percent, intensity);
            wb = morph_step(b, mb, percent, intensity);
            
            morphlut[i] = PackPixel(wr, wg, wb);
        }
    } 
    else
    {   
        if (intensity==100 && percent==63)
        {
            UpdatePalette(gamepal);
            return;
        }

        rgb[0]=mr; rgb[1]=mg; rgb[2]=mb;
        
        for (n = 0; n<3; n++)
        {
            if (rgb[n] < 0)
                rgb[n] = 0;
            else if (rgb[n] > 63)
                rgb[n] = 63;
        }
        
        // pull the colors
        for (n = 0; n<3*256; n++)
        {
            pmorph_palette[n] = morph_step(gamepal[n], rgb[n % 3], percent, intensity);
        }
        
        // enforce new palette
        UpdatePalette(pmorph_palette);
    }
}

bool IsMMX(void)
{
  return true;
}
