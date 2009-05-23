CFLAGS = -g -Wall `sdl-config --cflags` -O
LIBS = -g -Wall `sdl-config --libs` -lm -lSDL_image -lSDL_gfx -lSDL_ttf -lglut -lGLU -I /usr/local/include

basemmo: basemmo.o alex.o
	$(CC) $(CFLAGS) -o basemmo basemmo.o alex.o $(LIBS)

texgen: texgen.o alex.o
	$(CC) $(CFLAGS) -o texgen texgen.o alex.o $(LIBS)

clean:
	rm -f *~ basemmo texgen
	rm -f *.o
