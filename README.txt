v2.6 revision 6
===============

This was a fun project, to say the least.

Basically, this is a very heavy, and deep refactoring of v2.6's codebase, converting
subsystems to objects and generally taking advantage of C++'s superior semantic spectrum.
This package stands as a testament to the fact that v2's code, however awful, is not
beyond repair.  The task is not finished as of this writing, but it is well underway.
I'd guess that about two thirds of this code is purely my own.  Almost all of the other
third has been tweaked in some way or other.  I believe that a_memory.* and vfile.*
are the only subsystems that are still untouched, by either me or aen. (I don't think
they've changed at all since v2.0)  As of this writing, r6's codesize is fully five
thousand lines smaller than r5.

You won't find much has changed on the surface, which is what I had intended from the
start.  Things should be a teeny bit more stable, however.

As in revision 5, Audiere (fabulous high level audio API written by aegisknight)
is used for audio.  So, the sound formats supported are MOD, S3M, IT, XM, WAV, and OGG.
An interesting side effect is that you can use any of the same file formats for sound 
effects as well.  Currently, Audiere is a bit lacking in the SFX area, however, so a 
lot of sound effects won't play quite like they should.  Also, the panning control is 
currently disabled.

v2k's original image code is finally gone, replaced with Corona, a lovely little
library also written by aegis.  File formats supported are PCX, GIF, BMP, JPEG,
TGA, and PNG.  Screenshots are now always saved in PNG format instead of PCX or BMP.

Building Instructions
=====================

    Windows
    -------
    I have not tried to compile this with any other compiler other than MSVC6.  It
    can probably be made to work in minGW, (http://mingw.org) but it would be up
    to you to write the makefile. (send it to me if you pull it off, so I can
    add it to the distribution ^_^)  To compile it under MSVC6, simply open the
    project file and compile it.

    Linux and BSD
    -------------
	This hasn't been tested in quite some time.  I'm quite certain it's broken
	now, as I have created more than a few new source files, and removed a few 
	as well.  Maybe, if I find myself running linux again, I'll try to fix it.
	The task isn't so much porting the engine, as updating the makefile,
	and/or autoconf/Scons/what have you.

	If somebody else would like to try their hand at this task, I'd be happy
	to include it in this archive.

    SDL (http://libsdl.org), Audiere (http://audiere.sf.net), and Corona (http://corona.sf.net)
	are required for both platforms.

 -- Andy Friesen (tsb@verge-rpg.com)
