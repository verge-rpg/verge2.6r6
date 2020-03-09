
#ifndef _2XSAI_H
#define _2XSAI_H

#include "vtypes.h"

bool Init_2xSaI(u32 BitFormat);

void Super2xSaI(u8 *srcPtr, u32 srcPitch,
	     u8 *deltaPtr,
	     u8 *dstPtr, u32 dstPitch, int width, int height);

void SuperEagle(u8 *srcPtr, u32 srcPitch,
	     u8 *deltaPtr,
		 u8 *dstPtr, u32 dstPitch, int width, int height);

void _2xSaI(u8 *srcPtr, u32 srcPitch,
	     u8 *deltaPtr,
	     u8 *dstPtr, u32 dstPitch, int width, int height);

#endif
