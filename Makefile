PKGS=sdl gl SDL_image SDL_gfx SDL_ttf glu
CFLAGS = -g -Wall `pkg-config --cflags $(PKGS)`
LIBS = `pkg-config --libs $(PKGS)` -lm -lglut

basemmo: basemmo.o alex.o
	$(CC) $(CFLAGS) -o basemmo basemmo.o alex.o $(LIBS)

random: random.o alex.o
	$(CC) $(CFLAGS) -o random random.o alex.o $(LIBS)

clean:
	rm -f *~ basemmo random
	rm -f *.o
