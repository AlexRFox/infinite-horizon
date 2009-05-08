#include "alex.h"
#include "alexsdl.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

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
