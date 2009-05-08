#include "alexgl.h"

void
init_gl (int *argc, char **argv)
{
	qobj[SOLID] = gluNewQuadric ();
	gluQuadricDrawStyle (qobj[SOLID], GLU_FILL);
	qobj[WIRE] = gluNewQuadric ();
	gluQuadricDrawStyle (qobj[WIRE], GLU_LINE);
	
	glShadeModel (GL_SMOOTH);

	glutInit (argc, argv);

	libview.dist = 10;
	libview.phi = M_PI / 4;
}

void
init_sdl_gl_flags (int width, int height, Uint32 flags)
{
	if (SDL_Init (SDL_INIT_JOYSTICK | SDL_INIT_VIDEO) != 0) {
		printf ("Error: %s\n", SDL_GetError ());
		exit (1);
	}

	atexit (SDL_Quit);

	srandom (time (NULL));

	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 1);

	SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 6);
	SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 5);

	if (SDL_SetVideoMode (width, height, 16, SDL_OPENGL | flags) == NULL) {
		printf ("Error: %s\n", SDL_GetError ());
		exit(1);
	}
}

void grid (int lights, float color, int colors)
{
	float normal_color[3];

	if (lights)
		glDisable (GL_LIGHTING);
	
	normal_color[0] = color;
	normal_color[1] = color;
	normal_color[2] = color;

	glBegin (GL_LINES);
	if (colors)
		glColor3f (0, 0, 1);
	glVertex3f (-5, 5, 0);
	glVertex3f (5, 5, 0);
	glColor3fv (normal_color);
	glVertex3f(-5, 4, 0);
	glVertex3f(5, 4, 0);
	glVertex3f(-5, 3, 0);
	glVertex3f(5, 3, 0);
	glVertex3f(-5, 2, 0);
	glVertex3f(5, 2, 0);
	glVertex3f(-5, 1, 0);
	glVertex3f(5, 1, 0);
	if (colors)
		glColor3f(1, 0, 0);
	glVertex3f(-5, 0, 0);
	glVertex3f(5, 0, 0);
	glColor3fv(normal_color);
	glVertex3f(-5, -1, 0);
	glVertex3f(5, -1, 0);
	glVertex3f(-5, -2, 0);
	glVertex3f(5, -2, 0);
	glVertex3f(-5, -3, 0);
	glVertex3f(5, -3, 0);
	glVertex3f(-5, -4, 0);
	glVertex3f(5, -4, 0);
	if (colors)
		glColor3f(1, 1, 0);
	glVertex3f(-5, -5, 0);
	glVertex3f(5, -5, 0);
	glEnd();
	glBegin(GL_LINES);
	glColor3fv(normal_color);
	glVertex3f(-5, 5, 0);
	glVertex3f(-5, -5, 0);
	glVertex3f(-4, 5, 0);
	glVertex3f(-4, -5, 0);
	glVertex3f(-3, 5, 0);
	glVertex3f(-3, -5, 0);
	glVertex3f(-2, 5, 0);
	glVertex3f(-2, -5, 0);
	glVertex3f(-1, 5, 0);
	glVertex3f(-1, -5, 0);
	if (colors)
		glColor3f(0, 1, 0);
	glVertex3f(-0, 5, 0);
	glVertex3f(-0, -5, 0);
	glColor3fv(normal_color);
	glVertex3f(1, 5, 0);
	glVertex3f(1, -5, 0);
	glVertex3f(2, 5, 0);
	glVertex3f(2, -5, 0);
	glVertex3f(3, 5, 0);
	glVertex3f(3, -5, 0);
	glVertex3f(4, 5, 0);
	glVertex3f(4, -5, 0);
	glVertex3f(5, 5, 0);
	glVertex3f(5, -5, 0);
	glEnd();
	if (lights)
		glEnable (GL_LIGHTING);

}

void
distanceLookAt (double x, double y, double z, double dist, double theta,
		double phi)
{
	double pos[3];
	int zaxis = 1;
	int intphi;
	double floatphi;

	theta = RTOD (theta);
	phi = RTOD (phi);

	intphi = (int) phi;
	floatphi = phi - intphi;

	intphi = intphi % 360;

	phi = intphi + floatphi;

	if (abs(phi) > 90) {
		zaxis = -1;
		if (abs(phi) > 270) {
			zaxis = 1;
		}
	}
			
	theta = DTOR(theta);
	phi = DTOR(phi);

	if (dist < 0)
		dist = 0;

	pos[0] = x + dist * cos (theta) * cos (phi);
	pos[1] = y + dist * sin (theta) * cos (phi);
	pos[2] = z + dist * sin (phi);

	gluLookAt (pos[0], pos[1], pos[2],
		   x, y, z,
		   0, 0, zaxis);
}

void
color_coords (int lights)
{
	if (lights)
		glDisable (GL_LIGHTING);

	glBegin (GL_LINES);
	glColor3f (1, 0, 0);
	glVertex3f (0, 0, 0);
	glVertex3f (30, 0, 0);
	glEnd ();

	glBegin (GL_LINES);
	glColor3f (0, 1, 0);
	glVertex3f (0, 0, 0);
	glVertex3f (0, 30, 0);
	glEnd ();

	glBegin (GL_LINES);
	glColor3f (0, 0, 1);
	glVertex3f (0, 0, 0);
	glVertex3f (0, 0, 30);
	glEnd ();

	if (lights)
		glEnable (GL_LIGHTING);
}
