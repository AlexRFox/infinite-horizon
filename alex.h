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
#include <fenv.h>

#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#define DTOR(x) (x / 360.0 * 2 * M_PI)
#define RTOD(x) (x * 360.0 / 2 / M_PI)

#define LOGVAR_NSAMPLES 10000

enum {
	SOLID,
	WIRE,
};
GLUquadricObj *qobj[2];

SDL_Surface *screen;
TTF_Font *font;

struct logvar {
	struct logvar *next;
	char *name;
	double *valp;
	double samples[LOGVAR_NSAMPLES];
};
struct logvar *logvars;
int logvar_idx;
double logvar_times[LOGVAR_NSAMPLES];

struct vect {
	double x, y, z;
};

struct pt {
	double x, y, z;
};

struct plane {
	double a, b, c, d;
	struct pt middle;
};

struct view {
	double pos[3];
	double dist;
	double theta, phi;
} libview;

struct camera {
	double theta, theta_difference, phi;
};

struct camera playercamera;


void * xcalloc (int a, int b);
double get_secs (void);
double get_usecs (void);
double d_to_r (double degrees);
int alexsdl_init (int width, int height, Uint32 flags);
int alexttf_init (char *setfont, double fontsize);
void draw_text (char *string, TTF_Font *font, double x, double y, Uint32 color);
void load_blit (SDL_Surface **image, char *string);
void draw_blit (SDL_Surface *image, int x, int y);
void init_gl (int *argc, char **argv);
void init_sdl_gl_flags (int width, int height, Uint32 flags);
void grid (int lights, float color, int colors);
void distanceLookAt (double x, double y, double z, double dist, double theta,
		     double phi);
void color_coords (int lights);
void makeTexture (GLuint texName, int ImageSize, GLubyte ***Image);
double vdot (struct vect *v2, struct vect *v3);
void vcross (struct vect *v1, struct vect *v2, struct vect *v3);
void vadd (struct vect *v1, struct vect *v2, struct vect *v3);
void vsub (struct vect *v1, struct vect *v2, struct vect *v3);
void vscal (struct vect *v1, struct vect *v2, double s);
double square (double a);
double hypot3 (double a, double b, double c);
double hypot3v (struct vect *v);
double hypotv (struct vect *v);
void psub (struct vect *v1, struct pt *p1, struct pt *p2);
void vnorm (struct vect *v1, struct vect *v2);
void arrayvcross (double *a, double *b, double *c);
void arrayvsub (double *a, double *b, double *c);
void addvtopt (struct pt *p1, struct pt *p2, struct vect *v);
void vset (struct vect *v1, double x, double y, double z);
void write_logvars (void);
void add_logvar (char *name, double *valp);
void capture_logvars (void);
void vprint (struct vect *v);
void pset (struct pt *p, double x, double y, double z);
double dist_pt_to_plane (struct pt *p, struct plane *pl);
double z_at_pt_on_plane (struct pt *p, struct plane *pl);
void pt_on_z_plane (struct pt *p1, struct pt *p2, struct plane *plane);
void gauss_e3x3 (struct pt *pt, struct plane *pl1,
		 struct plane *pl2, struct plane *pl3);
