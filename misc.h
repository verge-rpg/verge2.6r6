
#ifndef MISC_H
#define MISC_H

#include "vtypes.h"
#include <string>
#include <vector>
#include <algorithm>

char* va(char* format, ...);

namespace RLE
{
	template <typename T>
	bool Read(T* dest, int len, u8* src)
	{
		do
		{
			int run = 1;
			u8 w = *src++;
			if (w == 0xFF)
			{
				run = *src++;
				w = *src++;
			}
			len -= run;

			if (len < 0)		// totally bogus. shaa.
				return false;

			while (run--)
				*dest++ = w;
		}
		while (len>0);

		return true;			// hurray.
	}

    template <>
    bool Read<bool>(bool* dest, int len, u8* src);

	template <typename T>
	bool Read(T* dest, int len, bool* src)
	{
		return Read(dest, len, (u8*)src);
	}

	template <typename T>
	bool Read(T* dest, int len, u16* src)
	{
		do
		{
			int run = 1;
			u16 w = *src++;
			if ((w & 0xFF00) == 0xFF00)
			{
				run = (u16)(w & 0x00FF);
				w = *src++;
			}
			len -= run;
			// totally bogus. shaa.
			if (len < 0)
				return false;
			while (run--)
				*dest++ = w;
		}
		while (len);

		// good
		return true;
	}

    template <typename T>
    bool Read(T* dest, int len, u32* src)
    {
        u16* temp = new u16[len];
        bool result = Read(temp, len, (u16*)src);
        if (!result)
            return false;

        /* std::copy rules memcpy. :D
         * Copying from a u16* to whatever dest is.  Typically
         * it'll be used for u32s.  So it stretches everything
         * out all neat and prettylike.
         */
        std::copy(temp, temp+len, dest);

        delete[] temp;
        return true;
    }
}

template <class T>
class ScopedPtr
{
    T* _data;
    ScopedPtr(ScopedPtr&){} // no copy!

public:
    ScopedPtr(T* t)  : _data(t) {}
    ScopedPtr()      : _data(0) {}
    ~ScopedPtr()     { delete _data;  }

    T* operator ->() const { return  _data; }
    T& operator * () const { return *_data; }
    T* get()         const { return  _data; }

    operator bool () { return _data != 0; }
  
    ScopedPtr& operator = (T* t)
    {
        delete _data;
        _data = t;
        return *this;
    }
};

template <class T>
class ScopedArray
{
    T* _data;
    ScopedArray(ScopedArray&){} // no copy!

public:
    ScopedArray(T* t)  : _data(t) {}
    ScopedArray()      : _data(0) {}
    ~ScopedArray()     { delete[] _data;  }

    T& operator[](uint i) const { return _data[i]; }
    T* operator ->()      const { return  _data; }
    T& operator * ()      const { return *_data; }
    T* get()              const { return  _data; }

    operator bool()       const { return _data != 0; }
  
    ScopedArray& operator = (T* t)
    {
        delete[] _data;
        _data = t;
        return *this;
    }
};

using std::min;
using std::max;

std::string trimString(const std::string& s);
std::vector<std::string> splitString(const std::string& s, const std::string& delimiters=" \n\r\t");
std::string stripExtension(const std::string& s);
std::string getExtension(const std::string& s);
std::string getPath(const std::string& s);
std::string getFilename(const std::string& s);
std::string lowerCase(const std::string& s);

#endif
