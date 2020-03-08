// Keyboard handling stuff.

// This was originally meant to mimic aen's keyboard handler, but has since
// been reversed. While aen's handler "feeds" keys to processes, this one
// simply puts them in a queue for the process to grab on it's own.

// ChangeLog
// + <tSB> 11.05.00 - Initial writing
// + <tSB> 11.07.00 - DirectInput won't compile right.  Re-started, using Win32's messaging system to handle keypresses.
// + <tSB> 11.07.00 - This doesn't work either :P Different DirectX libs work now. ^_^
// + <tSB> 11.09.00 - woo, another overhaul.  Based on vecna's Winv1/Blackstar code.
// + <tSB> 11.16.00 - Mouse code added
// + <tSB> 12.05.00 - Mouse code rehashed, using DInput again.
// + <tSB> 12.01.01 - Rewrite again.  SDL

#include "vtypes.h"
#include "input.h"
#include "keyboard.h"

namespace
{
/*	
	// Code used to make the table below.
	u8 tbl[256];
	
	for (int i=0; i<256; i++)
	{
		char c=getscan(i);
		tbl[i]=c;
	}
	
	for (int i=0; i<256; i++)
	{
		printf("%4i,",tbl[i]);
		if (!((i+1)&15))
			printf("\n");
	}
	printf("\n");
	exit(-1);*/

    // big ol' hack, so the behaviour stays the same.
    u8 ascii_to_scan[] =
    {
	   0,   0,   0,  59,   0,   0,   0,   0,  14,  15,   0,   0,   0,  28,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,
	  57,   0,   0,   0,   0,   0,   0,  40,   0,   0,  55,  78,  51,  12,  52,  53,
	  11,   2,   3,   4,   5,   6,   7,   8,   9,  10,   0,  39,   0,  13,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  26,  43,  27,   0,   0,
	  41,  30,  48,  46,  32,  18,  33,  34,  35,  23,  36,  37,  38,  50,  49,  24,
	  25,  16,  19,  31,  20,  22,  47,  17,  45,  21,  44,   0,   0,   0,   0,  83
    };

    u8 key_ascii_tbl[128] =
    {
          0,   0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  8,   9,
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',  13,   0,'a', 's',
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  39, '`',   0,  92, 'z', 'x','c', 'v',
        'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' ',   0,   3,   3,   3,  3,   8,
          3,   3,   3,   3,   3,   0,   0,   0,   0,   0, '-',   0,   0,   0,'+',   0,
          0,   0,   0, 127,   0,   0,  92,   3,   3,   0,   0,   0,   0,   0,  0,   0,
         13,   0, '/',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0, 127,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '/',   0,   0,   0,  0,   0
    };

    u8 key_shift_tbl[128] =
    {
        0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 126, 126,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 126, 0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 34,'~',   0,   '|', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   1,   0,   1,   1,   1,   1,   1,
        1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,
        0,   0,   1,   127, 0,   0,   0,   1,   1,   0,   0,   0,   0,   0,   0,   0,
        13,  0,   '/', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   127,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0
    };
};

#include <stdlib.h>

/*static char getscan(char c)
{
	for (int i=0; i<128; i++)
		if (key_ascii_tbl[i]==c)
			return i;
	return 0;
}*/

Input::Input()
{
}

Input::~Input()
{
}

int Input::Init()
{
    return 1;
}

void Input::ShutDown()
{
}

void Input::Update(SDLKey k,bool pressed) // updates things
{
    char c=0;

    switch (k)
    {
    case SDLK_LSHIFT:   c=42;   break;
    case SDLK_RSHIFT:   c=54;   break;
    case SDLK_UP:       c=72;   break;
    case SDLK_DOWN:     c=80;   break;
    case SDLK_LEFT:     c=75;   break;
    case SDLK_RIGHT:    c=77;   break;
    case SDLK_F1:       c=59;   break;
    case SDLK_F2:       c=60;   break;
    case SDLK_F3:       c=61;   break;
    case SDLK_F4:       c=62;   break;
    case SDLK_F5:       c=63;   break;
    case SDLK_F6:       c=64;   break;
    case SDLK_F7:       c=65;   break;
    case SDLK_F8:       c=66;   break;
    case SDLK_F9:       c=67;   break;
    case SDLK_F10:      c=68;   break;
    case SDLK_F11:      c=84;   break;
    case SDLK_F12:      c=85;   break;

default:                c=ascii_to_scan[k&127];
    }

    if (k==SDLK_RCTRL)  {   k=SDLK_LCTRL;   c=SCAN_LCONTROL;    }
    if (k==SDLK_RALT)   {   k=SDLK_LALT;    c=SCAN_LALT;        }

    key[c]=pressed;
    if (pressed)
        last_pressed=c;

    if (!c)
        printf("Key #%i '%s' (%i) was %s.\n",(int) k,SDL_GetKeyName(k),(int)c,pressed?"pressed":"released");

    if (pressed && kb_end!=kb_start+1)
        key_buffer[kb_end++]=c;

    switch (k)
    {
    case SDLK_UP:       up=pressed;     break;
    case SDLK_DOWN:     down=pressed;   break;
    case SDLK_LEFT:     left=pressed;   break;
    case SDLK_RIGHT:    right=pressed;  break;

    case SDLK_RETURN:   b1=pressed;     break;
    case SDLK_LALT:     b2=pressed;     break;
    case SDLK_ESCAPE:   b3=pressed;     break;
    case SDLK_SPACE:    b4=pressed;     break;
    }
    
    if (unpress[1])  {   if (b1   ) b1=0;    else unpress[1]=0;  }
    if (unpress[2])  {   if (b2   ) b2=0;    else unpress[2]=0;  }
    if (unpress[3])  {   if (b3   ) b3=0;    else unpress[3]=0;  }
    if (unpress[4])  {   if (b4   ) b4=0;    else unpress[4]=0;  }
    
    if (unpress[5])  {   if (up   ) up=0;    else unpress[5]=0;  }
    if (unpress[6])  {   if (down ) down=0;  else unpress[6]=0;  }
    if (unpress[7])  {   if (left ) left=0;  else unpress[7]=0;  }
    if (unpress[8])  {   if (right) right=0; else unpress[8]=0;  }
    
    UpdateMouse();
}

int Input::GetKey()
// gets the next key from the buffer, or 0 if there isn't one
{
    if (kb_start==kb_end) return 0; // nope!  nuthin here
    
    return key_buffer[kb_start++];
}

void Input::ClearKeys()
// clears the keyboard buffer (duh!)
{
    kb_end=kb_start=0;
}

void Input::UnPress(int control)
{
    switch(control)
    {
        // GROSS!
    case 0:
        if (b1) unpress[1]=1;
        if (b2) unpress[2]=1;
        if (b3) unpress[3]=1;
        if (b4) unpress[4]=1;
        
        if (up) unpress[5]=1;
        if (down) unpress[6]=1;
        if (left) unpress[7]=1;
        if (right) unpress[8]=1;
        b1=0; b2=0; b3=0; b4=0;
        left=0; right=0; up=0; down=0;
        break;
    case 1: if (b1) { unpress[1]=1; b1=0; } break;
    case 2: if (b2) { unpress[2]=1; b2=0; } break;
    case 3: if (b3) { unpress[3]=1; b3=0; } break;
    case 4: if (b4) { unpress[4]=1; b4=0; } break;
        
    case 5: if (up) unpress[5]=1; break;
    case 6: if (down) unpress[6]=1; break;
    case 7: if (left) unpress[7]=1; break;
    case 8: if (right) unpress[8]=1; break;
    }
}

char Input::Scan2ASCII(int scancode)
{
    return key_ascii_tbl[scancode];
}

void Input::MoveMouse(int x,int y)
{
    mousex=x; mousey=y;
}

void Input::UpdateMouse()
{
    mouseb=SDL_GetMouseState(&mousex,&mousey);
    
    if (mousex<mclip.left)  mousex=mclip.left;
    if (mousex>mclip.right) mousex=mclip.right;
    if (mousey<mclip.top)   mousey=mclip.top;
    if (mousey>mclip.bottom) mousey=mclip.bottom;
}

void Input::ClipMouse(const SRect& r)
{
    mclip=r;
}
