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

#ifndef VC_H
#define VC_H

#include "vtypes.h"

extern int			v2_touchy;		// exit to system at first sign of trouble?

extern int			mapevents;		// number of map events in this VC

extern int			hookretrace;
extern int			hooktimer;

extern int			stralloc;

extern void RunSystemAutoexec();

struct FuncDecl
{
	char fname[40];
	char argtype[20];
	int numargs, numlocals;
	int returntype;
	int syscodeofs;
};

struct StrDecl
{
	char vname[40];
	int vsofs;
	int arraylen;
};

struct VarDecl
{
	char vname[40];
	int varstartofs;
	int arraylen;
};

extern int		invc;
extern char*	mapvc;
extern char		vckill;

//extern u32*	vcsp;
//extern u32*	vcstack;

void LoadSystemVC();
void LoadMapVC(struct VFILE* f);
void ResetVC();                     // clears the callstack, etc etc
void ReadVCVar();
void WriteVCVar();
void ExecuteEvent(int);
void ExecuteUserFunc(int);

void CheckHookTimer();
void HookRetrace();
void HookTimer();
void HookKey(int script);

#endif // VC_H
