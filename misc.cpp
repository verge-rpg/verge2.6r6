#include <stdarg.h> // va_*()
#include <stdio.h>

//////////////////////////////////////////////////////////
// String manupulation. (TODO: get rid of all this)     //
//////////////////////////////////////////////////////////

int sgn(int x)
{
    if (x>0)
        return 1;
    else if (x<0)
        return -1;
    return 0;
}

char tolower(char c)
{
	if (c >= 'A' && c <= 'Z')
		c |= 32;
	return c;
}

#ifndef WIN32
char* strlwr(char* str)
{
	while (*str)
	{
		*str=tolower(*str);

		str++;
	}
	return str;
}

int strcasecmp(const char *s1, const char *s2)
{
	while (1)
	{
		if (tolower(*s1) < tolower(*s2))
			return -1;
		if (tolower(*s1) > tolower(*s2))
			return +1;
		if (!*s1 && !*s2)
			return 0;
		if (!*s1)
			return -1;
		if (!*s2)
			return +1;
		
		s1++;
		s2++;
	}
	return 666;
}
#endif

char* va(char* format, ...)
{
    va_list argptr;
    static char string[1024];
    
    va_start(argptr, format);
    vsprintf(string, format, argptr);
    va_end(argptr);
    
    return string;
}


