#include "alexsdl.h"

int
alexsdl_init (int width, int height, Uint32 flags)
{
        if (SDL_Init (SDL_INIT_VIDEO) != 0) {
		printf ("Unable to initialize SDL: %s\n", SDL_GetError());
		return (1);
	}

	screen = SDL_SetVideoMode (width, height, 32, flags);
	if (screen == NULL) {
		printf ("Unable to set video mode: %s\n", SDL_GetError());
		return (1);
	}
	return 0;
}

int
alexttf_init (char *setfont, double fontsize)
{
	TTF_Init ();
	font = TTF_OpenFont (setfont, fontsize);

	if (font == NULL) {
		printf ("Unable to locate or set font");
		return (1);
	}
	return 0;
}

void
draw_text (char *string, TTF_Font *font, double x, double y, Uint32 color)
{
	SDL_Color sdlcolor;
	SDL_Rect destination;
	SDL_Surface *text;

	SDL_GetRGB (color, screen->format, &(sdlcolor.r), &(sdlcolor.g),
		    &(sdlcolor.b));
	sdlcolor.unused = 0;

	destination.x = x;
	destination.w = 0;
	destination.y = y;
	destination.h = 0;

	text = TTF_RenderText_Solid (font, string, sdlcolor);
	SDL_BlitSurface (text, NULL, screen, &destination);
	SDL_FreeSurface (text);
}

void
load_blit (SDL_Surface **image, char *string)
{
	*image = SDL_DisplayFormat (IMG_Load (string));
	if (image == NULL) {
		printf ("WAAAA! Image didn't load!\n");
		exit (1);
	}
	SDL_SetColorKey (*image, SDL_SRCCOLORKEY, 0xff00ff);
}

void
draw_blit (SDL_Surface *image, int x, int y)
{
	SDL_Rect dest;

	dest.x = x;
	dest.y = y;

	SDL_BlitSurface (image, NULL, screen, &dest);
}
