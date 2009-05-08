#include "alex.h"

void *
xcalloc (int a, int b)
{
        void *p;

	if ((p = calloc (a, b)) == NULL) {
                fprintf (stderr, "memory error\n");
                exit (1);
        }
        return (p);
}

double
get_secs (void)
{
        struct timeval tv;
	gettimeofday (&tv, NULL);
        return (tv.tv_sec + tv.tv_usec/1e6);
}

double
d_to_r (double degrees)
{
        return (degrees / 360.0 * 2 * M_PI);
}

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

void
makeTexture (GLuint texName, int ImageSize, GLubyte ***Image)
{
	glBindTexture (GL_TEXTURE_2D, texName);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, ImageSize, ImageSize, 0,
		      GL_RGBA, GL_UNSIGNED_BYTE, Image);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

double
vdot (struct vect *v1, struct vect *v2)
{
	return ((v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z));
}

void
vcross (struct vect *v1, struct vect *v2, struct vect *v3)
{
	struct vect result;

	result.x = (v2->y * v3->z) - (v2->z * v3->y);
	result.y = (v2->z * v3->x) - (v2->x * v3->z);
	result.z = (v2->x * v3->y) - (v2->y * v3->x);

	*v1 = result;
}

void
vadd (struct vect *v1, struct vect *v2, struct vect *v3)
{
	v1->x = v2->x + v3->x;
	v1->y = v2->y + v3->y;
	v1->z = v2->z + v3->z;
}

void
vsub (struct vect *v1, struct vect *v2, struct vect *v3)
{
	v1->x = v2->x - v3->x;
	v1->y = v2->y - v3->y;
	v1->z = v2->z - v3->z;
}

void
vscalmul (struct vect *v1, struct vect *v2, double s)
{
	v1->x = v2->x * s;
	v1->y = v2->y * s;
	v1->z = v2->z * s;
}

void
vscaldiv (struct vect *v1, struct vect *v2, double s)
{
	v1->x = v2->x / s;
	v1->y = v2->y / s;
	v1->z = v2->z / s;
}

double
square (double a)
{
	return (a * a);
}

double
hypot3 (double a, double b, double c)
{
	return (sqrt (square (a) + square (b) + square (c)));
}

void
psub (struct vect *v1, struct pt *p1, struct pt *p2)
{
	v1->x = p1->x - p2->x;
	v1->y = p1->y - p2->y;
	v1->z = p1->z - p2->z;
}

void
vnorm (struct vect *v1, struct vect *v2)
{
	struct vect result;
	
	result.x = v2->x / hypot3 (v2->x, v2->y, v2->z);
	result.y = v2->y / hypot3 (v2->x, v2->y, v2->z);
	result.z = v2->z / hypot3 (v2->x, v2->y, v2->z);

	*v1 = result;
}

void
arrayvcross (double *a, double *b, double *c)
{
	// a = b cross c
	a[0] = b[1] * c[2] - b[2] * c[1];
	a[1] = b[2] * c[0] - b[0] * c[2];
	a[2] = b[0] * c[1] - b[1] * c[0];
}

void
arrayvsub (double *a, double *b, double *c)
{
	// a = b - c
	a[0] = b[0] - c[0];
	a[1] = b[1] - c[1];
	a[2] = b[2] - c[2];
}
