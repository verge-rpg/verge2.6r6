//hq2x filter demo program
//----------------------------------------------------------
//Copyright (C) 2003 MaxSt ( maxst@hiend3d.com )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later
//version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <windows.h>
#include "Image.h"

extern "C"
{
void hq2x_16(unsigned char*, unsigned char*, DWORD, DWORD, DWORD);
void hq2x_32(unsigned char*, unsigned char*, DWORD, DWORD, DWORD);
unsigned int   LUT16to32[65536];
unsigned int   RGBtoYUV[65536];
}

int InitLUTs(void)
{
  int i, j, k, r, g, b, Y, u, v;

  for (i=0; i<65536; i++)
    LUT16to32[i] = ((i & 0xF800) << 8) + ((i & 0x07E0) << 5) + ((i & 0x001F) << 3);

  for (i=0; i<32; i++)
  for (j=0; j<64; j++)
  for (k=0; k<32; k++)
  {
    r = i << 3;
    g = j << 2;
    b = k << 3;
    Y = (r + g + b) >> 2;
    u = 128 + ((r - b) >> 2);
    v = 128 + ((-r + 2*g -b)>>3);
    RGBtoYUV[ (i << 11) + (j << 5) + k ] = (Y<<16) + (u<<8) + v;
  }

  int nMMXsupport = 0;

  __asm
  {
    mov  eax, 1
    cpuid
    and  edx, 0x00800000
    mov  nMMXsupport, edx
  }

  return nMMXsupport;
}

int main(int argc, char* argv[])
{
  int         nBppOutput = 32;
  int         nRes;
  CImage      ImageIn;
  CImage      ImageOut;
  char      * szFilenameIn;
  char      * szFilenameOut;
  char      * szSwitch;
  
  if (argc <= 2)
  {
    printf("\nUsage: hq2x.exe input.bmp output.bmp\n");
    printf("supports .bmp and .tga formats\n");
    return 1;
  }

  szFilenameIn = argv[1];
  szFilenameOut = argv[2];

  if (argc > 3)
  {
    szSwitch = argv[3];
    if (strncmp( szSwitch, "/16", 3 ) == 0)
      nBppOutput = 16;
  }

  if ( GetFileAttributes( szFilenameIn ) == -1 )
  {
    printf( "ERROR: file '%s'\n not found", szFilenameIn );
    return 1;
  }

  if ( ImageIn.Load( szFilenameIn ) != 0 )
  {
    printf( "ERROR: can't load '%s'\n", szFilenameIn );
    return 1;
  }

  if ( ImageIn.m_BitPerPixel != 16 ) 
  {
    if ( ImageIn.ConvertTo16() != 0 )
    {
      printf( "ERROR: '%s' conversion to 16 bit failed\n", szFilenameIn );
      return 1;
    }
  }

  printf( "\n%s is %ix%i\n", szFilenameIn, ImageIn.m_Xres, ImageIn.m_Yres );

  if ( InitLUTs() == 0 )
  {
    printf( "ERROR: MMX is not supported.\n" );
    return 1;
  }

  if ( ImageOut.Init( ImageIn.m_Xres*2, ImageIn.m_Yres*2, nBppOutput ) != 0 )
  {
    printf( "ERROR: ImageOut.Init()\n" );
    return 1;
  };

  if ( nBppOutput == 16 )
    hq2x_16( ImageIn.m_pBitmap, ImageOut.m_pBitmap, ImageIn.m_Xres, ImageIn.m_Yres, ImageOut.m_Xres*2 );
  else
    hq2x_32( ImageIn.m_pBitmap, ImageOut.m_pBitmap, ImageIn.m_Xres, ImageIn.m_Yres, ImageOut.m_Xres*4 );

  nRes = ImageOut.Save( szFilenameOut );
  if ( nRes != 0 )
  {
    printf( "ERROR %i: ImageOut.Save(\"%s\")\n", nRes, szFilenameOut );
    return nRes;
  }
  printf( "%s is %ix%ix%i\n", szFilenameOut, ImageOut.m_Xres, ImageOut.m_Yres, ImageOut.m_BitPerPixel );

  printf( "\nOK\n" );
  return 0;
}
