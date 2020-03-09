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
// + <andy>11.07.03 - Joystick back.

#include "log.h"
#include "misc.h"
#include "vtypes.h"
#include "input.h"
#include "keyboard.h"

namespace
{
/*	
	// Code used to make the table below.
	u8 tbl[256];
	
	for (int i = 0; i<256; i++)
	{
		char c = getscan(i);
		tbl[i]=c;
	}
	
	for (int i = 0; i<256; i++)
	{
		printf("%4i, ", tbl[i]);
		if (!((i+1)&15))
			printf("\n");
	}
	printf("\n");
	exit(-1);
*/

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
          0,   0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '=', '-',  8,   9,
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',  13,   0, 'a', 's',
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  39, '`',   0,  92, 'z', 'x', 'c', 'v',
        'b', 'n', 'm', ', ', '.', '/',   0, '*',   0, ' ',   0,   3,   3,   3,  3,   8,
          3,   3,   3,   3,   3,   0,   0,   0,   0,   0, '-',   0,   0,   0, '+',   0,
          0,   0,   0, 127,   0,   0,  92,   3,   3,   0,   0,   0,   0,   0,  0,   0,
         13,   0, '/',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0, 127,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '/',   0,   0,   0,  0,   0
    };

    u8 key_shift_tbl[128] =
    {
        0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '+', '_', 126, 126,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 126, 0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 34, '~',   0,   '|', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   1,   0,   1,   1,   1,   1,   1,
        1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   '_', 0,   0,   0,   '+', 0,
        0,   0,   1,   127, 0,   0,   0,   1,   1,   0,   0,   0,   0,   0,   0,   0,
        13,  0,   '/', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   127,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '?', 0,   0,   0,   0,   0
    };
};

#include <stdlib.h>

/*static char getscan(char c)
{
	for (int i = 0; i<128; i++)
		if (key_ascii_tbl[i]==c)
			return i;
	return 0;
}*/

Input::Input()
    : joystick(0)
    , joyX(0)
    , joyY(1)
    , jb1(0)
    , jb2(1)
    , jb3(2)
    , jb4(3)
    , kb_start(0)
    , kb_end(0)
    , last_pressed(0)
    , mousex(0)
    , mousey(0)
    , mouseb(0)
    , up(false)
    , down(false)
    , left(false)
    , right(false)
    , b1(false)
    , b2(false)
    , b3(false)
    , b4(false)
{
    for (int i = 0; i < 9; i++) unpress[i] = false;
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
    if (joystick)
        SDL_JoystickClose(joystick);
}

void Input::EnableJoystick(int stick, int x, int y, int button1, int button2, int button3, int button4)
{
    joystick = SDL_JoystickOpen(stick);
    if (joystick)
    {
        joyX = x;
        joyY = y;
        jb1 = button1;
        jb2 = button2;
        jb3 = button3;
        jb4 = button4;
    }

    // TODO: validate indeces
}

void Input::Update()
{
    UpdateJoystick();
    //UpdateMouse();

    b1    = key[SCAN_ENTER] || joyState.b1;
    b2    = key[SCAN_ESC] || joyState.b2;
    b3    = key[SCAN_LALT] || joyState.b3;
    b4    = key[SCAN_LCONTROL] || joyState.b4;

    up    = key[SCAN_UP]    || joyState.up;
    down  = key[SCAN_DOWN]  || joyState.down;
    left  = key[SCAN_LEFT]  || joyState.left;
    right = key[SCAN_RIGHT] || joyState.right;
    
    if (unpress[1] && b1) b1 = false; else unpress[1] = false;
    if (unpress[2] && b2) b2 = false; else unpress[2] = false;
    if (unpress[3] && b3) b3 = false; else unpress[3] = false;
    if (unpress[4] && b4) b4 = false; else unpress[4] = false;

    if (unpress[5] && up)    up    = false; else unpress[5] = false;
    if (unpress[6] && down)  down  = false; else unpress[6] = false;
    if (unpress[7] && left)  left  = false; else unpress[7] = false;
    if (unpress[8] && right) right = false; else unpress[8] = false;
}

void Input::UpdateKeyboard(SDLKey k, bool pressed)
{
    u8 c = 0;

    switch (k)
    {
    case SDLK_LALT:     c = SCAN_LALT; break;
    case SDLK_RALT:     k = SDLK_LALT; c = SCAN_LALT; break;
    case SDLK_LCTRL:    c = SCAN_LCONTROL;  break;
    case SDLK_RCTRL:    c = SCAN_RCONTROL;  break;
    case SDLK_LSHIFT:   c = 42;   break;
    case SDLK_RSHIFT:   c = 54;   break;
    case SDLK_UP:       c = SCAN_UP;   break;
    case SDLK_DOWN:     c = SCAN_DOWN;   break;
    case SDLK_LEFT:     c = SCAN_LEFT;   break;
    case SDLK_RIGHT:    c = SCAN_RIGHT;   break;
    case SDLK_F1:       c = 59;   break;
    case SDLK_F2:       c = 60;   break;
    case SDLK_F3:       c = 61;   break;
    case SDLK_F4:       c = 62;   break;
    case SDLK_F5:       c = 63;   break;
    case SDLK_F6:       c = 64;   break;
    case SDLK_F7:       c = 65;   break;
    case SDLK_F8:       c = 66;   break;
    case SDLK_F9:       c = 67;   break;
    case SDLK_F10:      c = 68;   break;
    case SDLK_F11:      c = 84;   break;
    case SDLK_F12:      c = 85;   break;

    default:            c = ascii_to_scan[k&127];
    }

    if (k==SDLK_RCTRL)  {   k = SDLK_LCTRL;   c = SCAN_LCONTROL;    }

    key[c] = pressed;
    if (pressed)
        last_pressed = c;

    if (!c)
        printf("Key #%i '%s' (%i) was %s.\n", (int) k, SDL_GetKeyName(k), (int)c, pressed?"pressed":"released");

    if (pressed && kb_end!=kb_start+1)
        key_buffer[kb_end++]=c;

    /*switch (k)
    {
    case SDLK_UP:       up = pressed;     break;
    case SDLK_DOWN:     down = pressed;   break;
    case SDLK_LEFT:     left = pressed;   break;
    case SDLK_RIGHT:    right = pressed;  break;

    case SDLK_RETURN:   b1 = pressed;     break;
    case SDLK_LALT:     b2 = pressed;     break;
    case SDLK_ESCAPE:   b3 = pressed;     break;
    case SDLK_SPACE:    b4 = pressed;     break;
    }*/
}

void Input::UpdateJoystick()
{
    if (joystick)
    {
        s16 xAxis = SDL_JoystickGetAxis(joystick, joyX);
        s16 yAxis = SDL_JoystickGetAxis(joystick, joyY);

        // arbitrary threshhold
        joyState.left  = xAxis < -1024;
        joyState.right = xAxis >  1024;
        joyState.up    = yAxis < -1024;
        joyState.down  = yAxis >  1024;

        joyState.b1 = SDL_JoystickGetButton(joystick, jb1) != 0;
        joyState.b2 = SDL_JoystickGetButton(joystick, jb2) != 0;
        joyState.b3 = SDL_JoystickGetButton(joystick, jb3) != 0;
        joyState.b4 = SDL_JoystickGetButton(joystick, jb4) != 0;
    }
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
    kb_end = kb_start = 0;
}

void Input::UnPress(int control)
{
    switch(control)
    {
        // GROSS!
    case 0:
        if (b1) unpress[1] = true;
        if (b2) unpress[2] = true;
        if (b3) unpress[3] = true;
        if (b4) unpress[4] = true;
        
        if (up) unpress[5] = true;
        if (down) unpress[6] = true;
        if (left) unpress[7] = true;
        if (right) unpress[8] = true;
        b1 = false; b2 = false; b3 = false; b4 = false;
        left = false; right = false; up = false; down = false;
        break;
    case 1: unpress[1] = true; b1 = false; break;
    case 2: unpress[2] = true; b2 = false; break;
    case 3: unpress[3] = true; b3 = false; break;
    case 4: unpress[4] = true; b4 = false; break;
        
    case 5: unpress[5] = true; up = false; break;
    case 6: unpress[6] = true; down = false; break;
    case 7: unpress[7] = true; left = false; break;
    case 8: unpress[8] = true; right = false; break;
    }
}

char Input::Scan2ASCII(int scancode)
{
    if (key[SCAN_LSHIFT] || key[SCAN_RSHIFT])
        return key_shift_tbl[scancode];
    else
        return key_ascii_tbl[scancode];
}

void Input::MoveMouse(int x, int y)
{
    mousex = x; mousey = y;
}

void Input::UpdateMouse()
{
    mouseb = 0;
    int i = SDL_GetMouseState(&mousex, &mousey);
    if (i & SDL_BUTTON(0)) mouseb|=1;
    if (i & SDL_BUTTON(1)) mouseb|=2;
    if (i & SDL_BUTTON(2)) mouseb|=4;
    
    if (mousex<mclip.left)  mousex = mclip.left;
    if (mousex>mclip.right) mousex = mclip.right;
    if (mousey<mclip.top)   mousey = mclip.top;
    if (mousey>mclip.bottom) mousey = mclip.bottom;
}

void Input::ClipMouse(const SRect& r)
{
    mclip = r;
}
