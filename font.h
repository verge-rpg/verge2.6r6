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

#ifndef FONT_H
#define FONT_H

#include "vtypes.h"
#include "vfile.h"
#include <vector>
#pragma warning (disable:4786)

class CFont
{
    string_k        sFilename;
    
    int             nWidth, nHeight;        // font cell dimensions; currently all fonts are monospace
    int             nSubsets;               // total sets within this font
    int             nCursubset;             // active subset
    
    u8*             pData;                  // holds font image data
    int             nSize;
    
    bool LoadHeader(VFILE* vfp);             // helpers
public:
    bool IsVacant() const;
    void Clear();                           // vacates this font; frees image data, empties filename string
    
    // accessors
    const string_k& FileName()
        const { return sFilename; }
    int Height()        const { return nHeight; }
    int Width()         const { return nWidth; }
    int NumSubSets()    const { return nSubsets; }
    
    CFont();
    ~CFont();
    
    // primary interface
    
    int  CurSubSet() const      { return nSubsets; }
    void SetSubSet(int subset);
    
    void PrintChar(char ch,const int x,const int y);
    void Print(const char* zstr, int& x,int& y, int imbed = 0);
    
    bool LoadFromFile(const char* filename);
};
    
class CFontController
{
private:
    // actual fonts
    std::vector<CFont> font;
    int nCurx,nCury;
    int nAlignx;
    
public:
    // default constructor
    CFontController();
    
    int Load(const char* filename);
    
    // accessors
    CFont& operator[] (int slot) const;
    
    void GotoXY(int x,int y);
    
    void PrintString(int subset,const char* msg);

    int CurX() const { return nCurx; }
    int CurY() const { return nCury; }
};

#endif // FONT_H
