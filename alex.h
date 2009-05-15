#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/select.h>

#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#define DTOR(x) (x / 360.0 * 2 * M_PI)
#define RTOD(x) (x * 360.0 / 2 / M_PI)


SDL_Surface *screen;
TTF_Font *font;

struct vect {
	double x, y, z;
};

struct pt {
	double x, y, z;
};

void * xcalloc (int a, int b);

double get_secs (void);

double d_to_r (double degrees);

int alexsdl_init (int width, int height, Uint32 flags);

int alexttf_init (char *setfont, double fontsize);

void draw_text (char *string, TTF_Font *font, double x, double y, Uint32 color);

void load_blit (SDL_Surface **image, char *string);

void draw_blit (SDL_Surface *image, int x, int y);

enum {
	SOLID,
	WIRE,
};
GLUquadricObj *qobj[2];

void init_gl (int *argc, char **argv);
void init_sdl_gl_flags (int width, int height, Uint32 flags);

void grid (int lights, float color, int colors);
void distanceLookAt (double x, double y, double z, double dist, double theta,
		     double phi);
void color_coords (int lights);

struct view {
	double pos[3];
	double dist;
	double theta, phi;
} libview;

struct camera {
	double theta, theta_difference, phi;
};

struct camera playercamera;

void makeTexture (GLuint texName, int ImageSize, GLubyte ***Image);

double vdot (struct vect *v2, struct vect *v3);

void vcross (struct vect *v1, struct vect *v2, struct vect *v3);

void vadd (struct vect *v1, struct vect *v2, struct vect *v3);

void vsub (struct vect *v1, struct vect *v2, struct vect *v3);

void vscalmul (struct vect *v1, struct vect *v2, double s);

void vscaldiv (struct vect *v1, struct vect *v2, double s);

double square (double a);

double hypot3 (double a, double b, double c);

double hypot3v (struct vect *v);

double hypotv (struct vect *v);

void psub (struct vect *v1, struct pt *p1, struct pt *p2);

void vnorm (struct vect *v1, struct vect *v2);

void arrayvcross (double *a, double *b, double *c);

void arrayvsub (double *a, double *b, double *c);

void applyforcetopt (struct pt *p, struct vect *v);

void vinvert (struct vect *v1, struct vect *v2);

void zerovect (struct vect *v1);
