CFLAGS = -g -Wall `sdl-config --cflags`
LIBS = -g -Wall `sdl-config --libs` -lm -lSDL_image -lSDL_gfx -lSDL_ttf -lglut -lGLU

basemmo: basemmo.o alex.o
	$(CC) $(CFLAGS) -o basemmo basemmo.o alex.o $(LIBS)

random: random.o alex.o
	$(CC) $(CFLAGS) -o random random.o alex.o $(LIBS)

clean:
	rm -f *~ basemmo random
	rm -f *.o
