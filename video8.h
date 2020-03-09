#ifndef VIDEO8_H
#define VIDEO8_H

class Video8
{
    Video8(int xres, int yres, int bpp, bool fullscreen);
    ~Video8();

    void SetMode(int xres, int yres, int bpp);
    void SetVSync(bool set);

    void SetPalette(u8* pal);
    u8*  GetPalette();

    int XRes() const;
    int YRes() const;

    char* Desc();
    void Clear();

    void  CopySprite(int x, int y, int width, int height, u8* src);
    void TCopySprite(int x, int y, int width, int height, u8* src);
    void  ScaleSprite(int x, int y, int width, int height, int stretchwidth, int stretchheight, u8* src);
    void TScaleSprite(int x, int y, int width, int height, int stretchwidth, int stretchheight, u8* src);
    void RotScale(int posx, int posy, int width, int height, float angle, float scale, u8* src);
    void  WrapBlit(int x, int y, int width, int height, u8* src);
    void TWrapBlit(int x, int y, int width, int height, u8* src);
    void BlitStipple(int x, int y, int colour);

    void  CopySpriteLucent(int x, int y, int width, int height, u8* src, int lucentmode);
    void TCopySpriteLucent(int x, int y, int width, int height, u8* src, int lucentmode);
    void  ScaleSpriteLucent(int x, int y, int iwidth, int iheight, int dwidth, int dheight, u8* src, int lucent);
    void TScaleSpriteLucent(int x, int y, int iwidth, int iheight, int dwidth, int dheight, u8* src, int lucent);
    void RotScaleLucent(int posx, int posy, int width, int height, float angle, float scale, u8* src, int lucent);
    void WrapBlitLucent(int x, int y, int width, int height, u8* src, int lucent);
    void TWrapBlitLucent(int x, int y, int width, int height, u8* src, int lucent);

    void SetPixel(int x, int y, int colour, int lucent);
    int  GetPixel(int x, int y);
    void HLine(int x, int y, int x2, int colour, int lucent);
    void VLine(int x, int y, int y2, int colour, int lucent);
    void Line(int x1, int y1, int x2, int y2, int colour, int lucent);
    void Rect(int x1, int y1, int x2, int y2, int colour, int lucent);
    void RectFill(int x1, int y1, int x2, int y2, int colour, int lucent);
    void Circle(int x, int y, int radius, int colour, int lucent);
    void CircleFill(int x, int y, int radius, int colour, int lucent);

    void FlatPoly(int x1, int y1, int x2, int y2, int x3, int y3, int colour);

    void TMapPoly(int x1, int y1, int x2, int y2, int x3, int y3,
        int tx1, int ty1, int tx2, int ty2, int tx3, int ty3,
        int tw, int th, u8 *img);

    void SetClipRect(struct SRect& clip);
    void RestoreRenderSettings();
    void SetRenderDest(int x, int y, u8* dest);

    void PaletteMorph(int r, int g, int b, int percent, int intensity);

private:
    // ...
};

#endif