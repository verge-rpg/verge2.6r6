/*
Copyright (C) 1998 BJ Eirich (aka vecna)
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// /---------------------------------------------------------------------\
// |                          The VERGE Engine                           |
// |              Copyright (C)1998 BJ Eirich (aka vecna)                |
// |                 Font Loading / Text Output module                   |
// \---------------------------------------------------------------------/

/*
mod log:

  <andy>
  whenever        2002    Rewrote.
  <tSB>
  8   December    2000    Removed BIOS font crap.  Win2k doesn't like it.
  10  Nov         2000    Tweaks here and there for Windows port.
  <aen>
  30  December    1999    Major revamp.
*/

#include "verge.h"
#include "misc.h"
#include <vector>

bool CFont::IsVacant() const
{
    return pData==0;
}

void CFont::Clear()
{
    delete[] pData;
    pData = 0;
}

CFont::CFont()
: pData(0), nSize(0)
{}

CFont::~CFont()
{
    delete[] pData;
}

// primary interface

void CFont::SetSubSet(int subset)
{
    if (subset<0 || subset>=nSubsets)
        return;
    nCursubset = subset;
}

void CFont::PrintChar(char ch, const int x, const int y)
{
    // validate character range
    if (ch < 32 && ch >= 127)
        ch = 32;
    
    // convert to font bay character offset
    int offset        = (nCursubset*96*nWidth*nHeight) + ((ch-32)*nWidth*nHeight);
    
    // paint the character
    gfx.TCopySprite( x, y, nWidth, nHeight, pData + (offset*gfx.bpp) );
}

void CFont::Print(const char* zstr, int& x, int& y, int embed)
{
    int startx = x;

    while (*zstr)
    {
        const unsigned char ch = *zstr++;
        
        // i don't likes it. implement font selection some other way.
        switch (ch)
        {
        case 126: SetSubSet(0); continue;
        case 128: SetSubSet(1); continue;
        case 129: SetSubSet(2); continue;
        case 130: SetSubSet(3); continue;
        }
        
        // allow tab and newlines codes to have meaning?
        if (embed)
        {
            if (ch=='\t')
            {
                const int xalign = 64;

                int chx = (x - xalign)/nWidth;
                x += (4 - (chx % 4))*nWidth;
                continue;
            }
            
            if (ch=='\n')
            {
                y += nHeight;
                x  = startx;
                continue;
            }
        }
        
        PrintChar(ch, x, y);
        x += nWidth;
    }
}

bool CFont::LoadHeader(VFILE* vfp)
{
    try
    {
        if (vfp == 0)                       throw "CFont::LoadHeader: NULL file handle passed.";
        
        char ver = vgetc(vfp);
        if (ver != 1)                       throw va("CFont::LoadHeader: %s: incorrect version number (reported %d).\n", sFilename.c_str(), ver);
        
        // set font dims
        nWidth = vgetw(vfp);
        if (nWidth < 1 || nWidth > 128)     throw va("CFont::LoadHeader: %s: bogus cell-width (reported %d).\n", sFilename.c_str(), nWidth);
        
        nHeight = vgetw(vfp);
        if (nHeight < 1 || nHeight > 128)   throw va("CFont::LoadHeader: %s: bogus cell-length (reported %d).\n", sFilename.c_str(), nHeight);
        
        // set subsets
        nSubsets = vgetw(vfp);
        nCursubset = 0;
        if (nSubsets < 1 || nSubsets > 4)   throw va("CFont::LoadHeader: %s: bogus subset count (reported %d).\n", sFilename.c_str(), nSubsets);
    }
    catch (const char* msg)
    {
        if (vfp)
            vclose(vfp);
        Sys_Error(msg);
        return false;
    }
    
    // success
    return true;
}

bool CFont::LoadFromFile(const char* filename)
{
    // set filename; stuff relies on this for error messages
    sFilename = filename;

    Log::Write(va("Loading %s", filename));
    
    VFILE* f = vopen(sFilename.c_str());
    if (!f)        // failure
    {
        Log::Write(va("CFont::loadfromfile: %s: unable to open", sFilename.c_str()));
        return false;
    }
    
    LoadHeader(f);                                      // get header info and setup some defaults -- ignore return value for now; f is gauranteed to exist anyway
    
    int nSize = nSubsets*96*(nWidth*nHeight);             // get cumulative size of all subsets in this font & resize data to match
    
    delete[] pData;
    pData = new u8[nSize];
    
    vread(pData, nSize, f);
    
    vclose(f);
    
    if (gfx.bpp>1)
    {
        u8* p8bpp = pData;
        
        pData = new u8[nSize*2];
        
        u16* newptr = (u16*)pData;
        for (int n = 0; n < nSize; n += 1)
        {
            newptr[n]
                = (p8bpp[n])  ?  (gfx.Conv8( p8bpp[n] ))  :  gfx.trans_mask;
        }
        
        delete[] p8bpp;

        nSize+=nSize;
    }
    
    return true;                                        // success
}

//----------------------------------------------------------------------

static const int MAX_FONTS = 10;

CFontController::CFontController()
{
    font.resize(MAX_FONTS);
}

int CFontController::Load(const char* filename)
{
    if (!filename)
        return -1;
    
    for (unsigned int i = 0; i<font.size(); i++)
    {
        // if the font has already been loaded, then just return it, instead of loading a copy
#ifdef WIN32
        // win32 isn't case sensitive about filenames
        if (lowerCase(font[i].FileName()) == lowerCase(filename))
#else
        if (font[i].FileName() == filename)
#endif
            return i;

        if (font[i].IsVacant())
        {
            font[i].LoadFromFile(filename);
            return i;
        }
    }
    
    // no room
    return -1;
}

// accessors
CFont& CFontController::operator[] (uint slot) const
{
    if (slot < 0 || slot >= font.size())    // invalid requests get the system font
        return (CFont&)font[0];
    
    return (CFont&)font[slot];                      // valid requests are fulfilled
}

void CFontController::GotoXY(int x, int y)
{
    nCurx = nAlignx = x;
    nCury = y;
}

void CFontController::PrintString(int subset, const char* msg)
{
    if (font[subset].IsVacant())
        Sys_Error("Invalid font request (%i)", subset);

    font[subset].Print(msg, nCurx, nCury, true);
}
