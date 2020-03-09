#include <stdarg.h> // va_*()
#include <stdio.h>

#include <string>

#include "misc.h"

namespace RLE
{
    template<>
    bool Read(bool* dest, int len, u8* src)
    {
        return Read((u8*)dest, len, src);
    }
}

int sgn(int x)
{
    if (x>0)
        return 1;
    else if (x<0)
        return -1;
    return 0;
}

//////////////////////////////////////////////////////////
// String manupulation. (TODO: get rid of all this)     //
//////////////////////////////////////////////////////////

char* va(char* format, ...)
{
    va_list argptr;
    static char str[1024];

    va_start(argptr, format);
    vsprintf(str, format, argptr);
    va_end(argptr);

    return str;
}

std::string trimString(const std::string& s) {
    std::string t;

    // Find the first nonwhitespace character
    for (unsigned int i = 0; i < s.length(); i++) {
        if (s[i] != ' ') {
            t = s.substr(i);
            break;
        }
    }

    // Find the last nonwhitespace character
    for (int i = t.length()-1; i >= 0; i++) {
        if (t[i] != ' ') {
            t = t.substr(0, i);
            return t;
        }
    }

    return ""; // if we've made it this far, then every character in the std::string is whitespace.
}

std::vector<std::string> splitString(const std::string& s, const std::string& delimiters) {
    std::vector<std::string> sv;
    unsigned int p = 0;
    int pos = 0;

    for (unsigned int i = 0; i < s.length(); i++) {
        if (( pos = delimiters.find(s[i]) ) != -1) {
            std::string st = s.substr(p, i - p);
            if (st.length() > 0) {
                sv.push_back(st);
            }

            p = i + 1;
            i++;
        }
    }

    if (p < s.length())
        sv.push_back(s.substr(p));      // Push whatever remains (if anything remains)

    return sv;
}

std::string stripExtension(const std::string& s) {
    int p = s.rfind('.');
    if (p != -1)
        return s.substr(0, p);
    else
        return s;
}

std::string getExtension(const std::string& s) {
    int p = s.rfind('.');
    if (p != -1)
        return s.substr(p);
    else
        return "";
}

std::string getPath(const std::string& s) {
    int p = s.rfind('/');
    if (p != -1)
        return s.substr(0, p + 1 );
    else
        return "";
}

std::string getFilename(const std::string& s) {
    int p = s.rfind('/');
    if (p != -1)
        return s.substr(p+1);
    else
        return s;
}

// suboptimal.  meh.
std::string lowerCase(const std::string& s) {
    std::string t;
    t.reserve(s.length());

    for (unsigned int i = 0; i<s.length(); i++)
    {
        char c = s[i];
        if (c>='A' && c<='Z')
            c|=32;
        t+=c;
    }
    return t;
}
