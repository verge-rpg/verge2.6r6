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

#ifndef KEYBOARD_H
#define KEYBOARD_H

// Constants for keyboard scan-codes -- copied from dinput.h

#define SCAN_ESC             0x01
#define SCAN_1               0x02
#define SCAN_2               0x03
#define SCAN_3               0x04
#define SCAN_4               0x05
#define SCAN_5               0x06
#define SCAN_6               0x07
#define SCAN_7               0x08
#define SCAN_8               0x09
#define SCAN_9               0x0A
#define SCAN_0               0x0B
#define SCAN_MINUS           0x0C    // - on main keyboard 
#define SCAN_EQUALS          0x0D
#define SCAN_BACK            0x0E    // backspace 
#define SCAN_TAB             0x0F
#define SCAN_Q               0x10
#define SCAN_W               0x11
#define SCAN_E               0x12
#define SCAN_R               0x13
#define SCAN_T               0x14
#define SCAN_Y               0x15
#define SCAN_U               0x16
#define SCAN_I               0x17
#define SCAN_O               0x18
#define SCAN_P               0x19
#define SCAN_LBRACKET        0x1A
#define SCAN_RBRACKET        0x1B
#define SCAN_ENTER           0x1C
#define SCAN_LCONTROL        0x1D
#define SCAN_A               0x1E
#define SCAN_S               0x1F
#define SCAN_D               0x20
#define SCAN_F               0x21
#define SCAN_G               0x22
#define SCAN_H               0x23
#define SCAN_J               0x24
#define SCAN_K               0x25
#define SCAN_L               0x26
#define SCAN_SEMICOLON       0x27
#define SCAN_APOSTROPHE      0x28
#define SCAN_GRAVE           0x29    // accent grave 
#define SCAN_LSHIFT          0x2A
#define SCAN_BACKSLASH       0x2B
#define SCAN_Z               0x2C
#define SCAN_X               0x2D
#define SCAN_C               0x2E
#define SCAN_V               0x2F
#define SCAN_B               0x30
#define SCAN_N               0x31
#define SCAN_M               0x32
#define SCAN_COMMA           0x33
#define SCAN_PERIOD          0x34    // . on main keyboard 
#define SCAN_SLASH           0x35    // / on main keyboard 
#define SCAN_RSHIFT          0x36
#define SCAN_MULTIPLY        0x37    // * on numeric keypad 
#define SCAN_LMENU           0x38    // left Alt 
#define SCAN_SPACE           0x39
#define SCAN_CAPITAL         0x3A
#define SCAN_F1              0x3B
#define SCAN_F2              0x3C
#define SCAN_F3              0x3D
#define SCAN_F4              0x3E
#define SCAN_F5              0x3F
#define SCAN_F6              0x40
#define SCAN_F7              0x41
#define SCAN_F8              0x42
#define SCAN_F9              0x43
#define SCAN_F10             0x44
#define SCAN_NUMLOCK         0x45
#define SCAN_SCROLL          0x46    // Scroll Lock 
#define SCAN_NUMPAD7         0x47
#define SCAN_NUMPAD8         0x48
#define SCAN_NUMPAD9         0x49
#define SCAN_SUBTRACT        0x4A    // - on numeric keypad 
#define SCAN_NUMPAD4         0x4B
#define SCAN_NUMPAD5         0x4C
#define SCAN_NUMPAD6         0x4D
#define SCAN_ADD             0x4E    // + on numeric keypad 
#define SCAN_NUMPAD1         0x4F
#define SCAN_NUMPAD2         0x50
#define SCAN_NUMPAD3         0x51
#define SCAN_NUMPAD0         0x52
#define SCAN_DECIMAL         0x53    // . on numeric keypad 
#define SCAN_OEM_102         0x56    // < > | on UK/Germany keyboards 
#define SCAN_F11             0x57
#define SCAN_F12             0x58
#define SCAN_HOME            0xC7    // Home on arrow keypad 
#define SCAN_UP              0xC8    // UpArrow on arrow keypad 
#define SCAN_PRIOR           0xC9    // PgUp on arrow keypad 
#define SCAN_LEFT            0xCB    // LeftArrow on arrow keypad 
#define SCAN_RIGHT           0xCD    // RightArrow on arrow keypad 
#define SCAN_END             0xCF    // End on arrow keypad 
#define SCAN_DOWN            0xD0    // DownArrow on arrow keypad 
#define SCAN_NEXT            0xD1    // PgDn on arrow keypad 
#define SCAN_INSERT          0xD2    // Insert on arrow keypad 
#define SCAN_DELETE          0xD3    // Delete on arrow keypad 
#define SCAN_LCONTROL        0x1D
#define SCAN_RCONTROL        0x9D
#define SCAN_RALT            0xB8    // right Alt 
#define SCAN_LALT            0x38    // left Alt 

#define SCAN_BACKSPACE       SCAN_BACK            // backspace 
#define SCAN_NUMPADSTAR      SCAN_MULTIPLY        // * on numeric keypad 
#define SCAN_CAPSLOCK        SCAN_CAPITAL         // CapsLock 
#define SCAN_NUMPADMINUS     SCAN_SUBTRACT        // - on numeric keypad 
#define SCAN_NUMPADPLUS      SCAN_ADD             // + on numeric keypad 
#define SCAN_NUMPADPERIOD    SCAN_DECIMAL         // . on numeric keypad 
#define SCAN_NUMPADSLASH     SCAN_DIVIDE          // / on numeric keypad 
#define SCAN_UPARROW         SCAN_UP              // UpArrow on arrow keypad 
#define SCAN_PGUP            SCAN_PRIOR           // PgUp on arrow keypad 
#define SCAN_LEFTARROW       SCAN_LEFT            // LeftArrow on arrow keypad 
#define SCAN_RIGHTARROW      SCAN_RIGHT           // RightArrow on arrow keypad 
#define SCAN_DOWNARROW       SCAN_DOWN            // DownArrow on arrow keypad 
#define SCAN_PGDN            SCAN_NEXT            // PgDn on arrow keypad 


#endif
