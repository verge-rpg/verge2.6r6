
#ifndef HQ3X_H
#define HQ3X_H

extern "C"
{
    void hq2x_16(unsigned char* src, unsigned char* dest, unsigned int width, unsigned int height, unsigned int pitch);
    void hq2x_32(unsigned char* src, unsigned char* dest, unsigned int width, unsigned int height, unsigned int pitch);
    void hq3x_16(unsigned char* src, unsigned char* dest, unsigned int width, unsigned int height, unsigned int pitch);
    void hq3x_32(unsigned char* src, unsigned char* dest, unsigned int width, unsigned int height, unsigned int pitch);
    void hq4x_16(unsigned char* src, unsigned char* dest, unsigned int width, unsigned int height, unsigned int pitch);
    void hq4x_32(unsigned char* src, unsigned char* dest, unsigned int width, unsigned int height, unsigned int pitch);
    extern unsigned int   LUT16to32[65536];
    extern unsigned int   RGBtoYUV[65536];
}

void InitLUTs();

#endif
