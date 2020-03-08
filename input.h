// Input module for v2.6 by the Speed Bump
// This stuff is all GPL.
// In short: there is NO WARRANTY on this, you can't keep it secret, and you can't steal credit from me.

#ifndef INPUT_H
#define INPUT_H

#include "SDL.h"
#include "vtypes.h"

class Input
{
private:
	
	// key buffer
	unsigned char key_buffer[256]; // remember up to 256 events
	unsigned char kb_start,kb_end; // start/end of keyboard buffer.  Since they're bytes, they automaticly wrap around.
	
	SRect mclip;
	
	// for UnPress
	u8 unpress[9];
	
public:
	u8 key[256]; // set to 1 if the key is down, 0 otherwise
	char last_pressed;      // last key event here
	
	// mouse stuff
	int mousex,mousey;        // mouse coordinates
	int mouseb;
	
	char up,down,left,right; // directional controls
	char b1,b2,b3,b4;        // virtual button thingies
	
	Input();  // constructor
	~Input(); // destructor
	
	int Init();
	void ShutDown();
	
	void Update(SDLKey k,bool pressed);
	void UnPress(int control);
	
	// Keyboard stuff
	int  GetKey();
	void ClearKeys();
	char Scan2ASCII(int scancode); // returns the ASCII code. ;)
	
	void MoveMouse(int x,int y);
	void UpdateMouse();
	void ClipMouse(const SRect& r);
};

#endif
