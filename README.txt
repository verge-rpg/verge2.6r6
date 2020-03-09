                             v2.6 revision 9
                             ===============

Release Notes
=============

This was a fun project, to say the least.

Basically, this is a very heavy, and deep refactoring of v2.6's codebase,
converting subsystems to objects and generally taking advantage of C++'s
superior semantic spectrum.  This package stands as a testament to the fact
that v2's code, however awful, was not beyond repair.  As of this writing,
everything except for the VC interpreter, VFILE, and a few other odds and
ends have been completely reimplemented. (vfile has not changed at all since
v2.0a, I believe)

You won't find much has changed on the surface, which is what I had intended
from the start.  Things should be more stable, however.

As of revision 5, Audiere (fabulous high level audio API written by 
aegisknight) is used for audio.  So, the sound formats supported are MOD, S3M, 
IT, XM, WAV, FLAC, MP3 and OGG.  An interesting side effect is that you can use 
any of the same file formats for sound effects as well.

v2k's original image code is finally gone, replaced with Corona, a lovely
little library also written by aegis.  File formats supported are PCX, GIF,
BMP, JPEG, TGA, and PNG.  Screenshots are now always saved in PNG format
instead of PCX or BMP.

user.cfg
========

Okay, enough is enough.  Definitive documentation is neat.

Each command in user.cfg must be at the beginning of a line of its own.
Whitespace and unrecognized commands will be ignored.

    mount <PackFileName>
    --------------------
    Mounts the specified packfile into the virtual file system.  Up to three
    packfiles can be mounted in this way. (this restriction is arbitrary and
    completely idiotic)

    vidmode <xres> <yres>
    ---------------------
    Sets the initial video resolution for the engine.  xres and yres must both
    be greater than zero.  If the engine is running fullscreen, then this must 
    be a resolution that the graphics hardware directly supports.

        Commonly supported video modes:

        320x240   400x300 (less common)
        640x480   800x600   
        1024x768  1280x1024 
        1600x1200
      
    Resolutions above 640x480 will probably be slow, unless you have an 
    extremely fast machine.

    log
    ---
    If this command is present in user.cfg, logging information will be 
    written to a text file named verge.log.  Log output is discarded if 
    this option is not specified.

    startmap <MapName>
    ------------------
    Sets the map to be loaded on startup.  If this is omitted, then the 
    engine will try to run a function named AutoExec, found in system.vc.  
    AutoExec must accept no arguments, and must return void.  If it is not 
    present, or does not meet these criteria, an error is reported.

    hicolor
    -------
    If present, v26 runs in 16bpp hicolour, instead of 8bpp.

    window
    ------
    v26 runs in a window on the desktop instead of fullscreen if this is
    set.
    
    filter <FilterName>
    -------------------
    Enables a video filter for output processing.  Filters are ignored completely
    in 8bpp mode.
    
    Available filters:
        normal - The default.  Does no filtering.  If you omit the filter
                 directive, it does this.
        eagle  - Kreed's Eagle algorithm.  The fastest filter, to my knowledge.
                 <http://elektron.its.tudelft.nl/~dalikifa/>
        2xsai  - 2x Scaling and Interpolation.  Also by Kreed.  Nicer looking
                 than eagle.  More taxing on the CPU as well.
        hq2x   - High Quality 2x scaling, implemented by MaxSt 
                 <http://www.hiend3d.com/>
                 Very nice quality, but significantly slower than Kreed's
                 filters.
        hq3x   - High Quality 3x.  Exorbitant.  Excessively CPU demanding.
        hq4x   - High Quality 4x.  Do NOT use this at any resolution above
                 320x240.  Doing even this requires a physical resolution of
                 1280x960.  Your processor will cry.

    vsync
    -----
    Enables vertical sync if possible.  This option decreases the framerate 
    somewhat, but eliminates "tearing".

    profile
    -------
    Turns the VC profiler on.  The profiler significantly decreases
    VC execution speed, but creates a document called vcprofile.htm that
    contains timing information.
    
    The profiler records how much time was spent executing each VC
    function.  It's useful for finding performance bottlenecks.

    joystick
    --------
    Enables the joystick.  If an integer argument is specified on the same
    line, it is used to indicate which joystick is used.  0 is the default.
    
    joyx
    ----
    Indicates which joystick axis to interpret as left/right movement.  0 is 
    the default, and should not usually need to be changed, unless you have 
    some sort of wizz bang gamepad with 8 axes on it, and you want to use a 
    hat switch for movement instead of the analog pad.  or something.
    
    This can usually be omitted altogeather.
    
    joyy
    ----
    Indicates which joystick axis to interpret as up/down movement.  1 is the
    default, and as with joyx, should not usually need to be modified.
    
    Like joyx, this can almost always be left out.
    
    jb1
    ---
    Index of the joystick button to interpret as virtual button 1.  This button
    is used to 'activate' things adjacent to the player.  If omitted, button 0
    is used.
    
    jb2, jb3, jb4
    -------------
    Indeces of the joystick buttons to interpret as virtual buttons 2, 3, and 4.
    These aren't used by v2's core itself, but are accessable by VC.  Season to
    taste.
    
    If omitted, buttons 1, 2, and 3 are used, respectively.

Building Instructions
=====================

    You can build v26 with either SCons (http://www.scons.org) or MS Visual
    C++ .NET.  In fact, I think you'll probably need VC.NET whether you use
    the SCons script or not.  I hope to make it work better with GCC, but
    only time may tell...

    Windows
    -------
    Project file.  Build from VS.NET GUI.  Or run scons.
    You'll need NASM to build the assembly stuff.  And some other hackery, since
    I haven't made the build semantics of that little corner very friendly just
    yet.

    Linux and BSD
    -------------
    Almost certainly broken.  Run scons.  Pray.

    If somebody else would like to try their hand at this task, I'd be happy to
    include it in this archive.

SDL (http://libsdl.org), Audiere (http://audiere.sf.net), and Corona
(http://corona.sf.net) are required for both platforms.

 -- Andy Friesen (andy@ikagames.com)
