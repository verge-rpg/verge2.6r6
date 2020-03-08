CC = gcc
CFLAGS = $(shell sdl-config --cflags) -DNO_SOUND
LDFLAGS = $(shell sdl-config --libs)
SOURCEFILES = a_memory.cpp  image.cpp	 startup.cpp   verge.cpp   \
	      console.cpp   render.cpp   strk.cpp    \
	      engine.cpp    linked.cpp	 graph.cpp     vfile.cpp   \
	      entity.cpp    sound.cpp    timer.cpp   \
	      input.cpp     font.cpp	 message.cpp   vc.cpp

EXECUTABLE = verge

build:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCEFILES) -o $(EXECUTABLE)

clean:
	rm -f *.o
	
all:	clean build
