#include "alex.h"
#include <fenv.h>

enum {
	WIDTH = 800,
	HEIGHT = 600,

	GRAVITY = -1,

	NO = 0,
	YES = 1,

	UP = 1,
	DOWN = 2,
	LEFT = 3,
	RIGHT = 4,
	Q = 5,
	E = 6,
	FAKE_Q = 7,
	FAKE_E = 8,

	FORW = 1,
	BACK = 2,
	SIDE_Q = 3,
	SIDE_E = 4,
	FORW_Q = 5,
	FORW_E = 6,
	BACK_Q = 7,
	BACK_E = 8,

	GROUNDED = 0,
	FALLING = 1,
	FLYING = 2,
};

int arrowkey[16], mousebutton[8], paused, gndcounter;
double mouse_x, mouse_y, oldmouse_x, oldmouse_y;
GLuint texName[3], terrain_list;

struct groundtexture {
	int texturesize;
	GLubyte *tex;
};

struct groundtexture groundtexture;

struct groundtri {
	struct groundtri *next;
	int gndnum;
	double a, b, c, d, theta;
	struct pt p1, p2, p3;
	struct vect normv, downhill;
};

struct groundtri *first_groundtri, *last_groundtri;

struct unit {
	int moving, movtype;
	double theta, turnspeed, camdist, speed, lasttime, mass;
	struct pt p;
	struct vect vel;
	struct groundtri *loc;
};

struct unit player;

void makeImages (void);
void read_terrain (void);
void define_plane (double x1, double y1, double z1,
		   double x2, double y2, double z2,
		   double x3, double y3, double z3,
		   double normx, double normy, double normz);
struct groundtri * detect_plane (struct pt *p);
double ground_height (struct pt *p);
void process_input (void);
void process_mouse (void);
void leftclickdown (void);
void middleclickdown (void);
void rightclickdown (void);
void leftclickup (void);
void middleclickup (void);
void rightclickup (void);
void zoomin (void);
void zoomout (void);
void movement (void);
void moving (void);
void grounded_moving (void);
void falling_moving (void);
void flying_moving (void);
void draw (void);

void
makeImages (void)
{
	FILE *f;
	char *filename = "                             ";
	char s[1000];
	int i, j;

	filename = "grass.ppm";
	if ((f = fopen (filename, "r")) == NULL) {
		printf ("Error, cannot open %s\n", filename);
		exit (1);
	}

	for (i = 0; i < 3; i++) {
		if (fgets (s, sizeof s, f) == NULL) {
			printf ("Something's wrong with the file, exiting\n");
			exit (1);
		}
		if (s[0] == '#')
			i--;
		if (i == 1) {
			strtok (s, " ");
			groundtexture.texturesize = atof (s);
		}
	}

	groundtexture.tex
		= xcalloc (groundtexture.texturesize
			   * groundtexture.texturesize * 4,
			   sizeof *groundtexture.tex);
	for (i = 0; i < groundtexture.texturesize; i++) {
		for (j = 0; j < groundtexture.texturesize; j++) {
			groundtexture.tex[i * groundtexture.texturesize * 4
					  + j * 4 + 0] = getc (f);
			groundtexture.tex[i * groundtexture.texturesize * 4
					  + j * 4 + 1] = getc (f);
			groundtexture.tex[i * groundtexture.texturesize * 4
					  + j * 4 + 2] = getc (f);
			groundtexture.tex[i * groundtexture.texturesize * 4
					  + j * 4 + 3] = 255;
		}
	}

	fclose (f);
}

void
read_terrain (void)
{
	FILE *inf;
	char buf[1000];
	int idx;
	char *tokstate, *tok;
	double vals[12], B_minus_A[3], C_minus_A[3], normal[3];

	if ((inf = fopen ("simpleground2.raw", "r")) == NULL) {
		fprintf (stderr, "can't open simpleground2.raw\n");
		exit (1);
	}

	terrain_list = glGenLists (1);
	
	glNewList (terrain_list, GL_COMPILE);

	glPushMatrix ();

	glEnable (GL_TEXTURE_2D);

	glColor3f (1, 1, 0);

	while (fgets (buf, sizeof buf, inf) != NULL) {
		idx = 0;
		tokstate = buf;
		while (idx < 9 && (tok = strtok (tokstate, " ")) != NULL) {
			tokstate = NULL;
			vals[idx++] = atof (tok);
		}

		arrayvsub (B_minus_A, vals + 3, vals);
		arrayvsub (C_minus_A, vals + 6, vals);
		arrayvcross (normal, B_minus_A, C_minus_A);

		if (idx == 9) {
			define_plane (vals[0], vals[1], vals[2],
				      vals[3], vals[4], vals[5],
				      vals[6], vals[7], vals[8],
				      normal[0], normal[1], normal[2]);

			glBindTexture (GL_TEXTURE_2D, texName[0]);
			glBegin (GL_TRIANGLES);
			glTexCoord2f (1, 0);
			glVertex3dv (vals);
			glTexCoord2f (1, 1);
			glVertex3dv (vals + 3);
			glTexCoord2f (0, 1);
			glVertex3dv (vals + 6);
			
			glNormal3dv (normal);
			
			glEnd ();
		} else {
			printf ("Model has non-triangles, exiting\n");
			exit (1);
		}
	}

	fclose (inf);
	
	glDisable (GL_TEXTURE_2D);
	
	glPopMatrix ();
	
	glEndList ();
}

void
define_plane (double x1, double y1, double z1,
	      double x2, double y2, double z2,
	      double x3, double y3, double z3,
	      double normx, double normy, double normz)
{
	struct groundtri *gp;
	struct vect v1, v2;
	
	gp = xcalloc (1, sizeof *gp);
	
	if (first_groundtri == NULL) {
		first_groundtri = gp;
	} else {
		last_groundtri->next = gp;
	}

	last_groundtri = gp;
	
	gp->a = normx;
	gp->b = normy;
	gp->c = normz;
	gp->d = -(gp->a * x1 + gp->b * y1 + gp->c * z1);
	
	gp->p1.x = x1;
	gp->p1.y = y1;
	gp->p1.z = z1;
	gp->p2.x = x2;
	gp->p2.y = y2;
	gp->p2.z = z2;
	gp->p3.x = x3;
	gp->p3.y = y3;
	gp->p3.z = z3;

	gp->normv.x = normx;
	gp->normv.y = normy;
	gp->normv.z = normz;

	vnorm (&gp->normv, &gp->normv);

	if (fabs (gp->c) < 1e-6) {
		gp->downhill.x = 0;
		gp->downhill.y = 0;
		gp->downhill.z = -1;
	} else if (hypot (gp->a, gp->b) < 1e-6) {
		gp->downhill.x = 1;
		gp->downhill.y = 0;
		gp->downhill.z = 0;
	} else {
		gp->downhill.x = gp->a;
		gp->downhill.y = gp->b;
		gp->downhill.z = (-gp->a * gp->a - gp->b * gp->b - gp->d)
			/ gp->c;
	}

	vnorm (&gp->downhill, &gp->downhill);
	
	vscal (&v1, &gp->downhill, -1);
	if (v1.x == 0 && v1.y == 0) {
		v2.x = 1;
		v2.y = 1;
		v2.z = 0;
	} else {
		v2.x = v1.x;
		v2.y = v1.y;
		v2.z = 0;
	}

	gp->theta = acos (vdot (&v1, &v2));

	gp->gndnum = gndcounter;
	gndcounter++;
}

struct groundtri *
detect_plane (struct pt *p1)
{
	int best_flag;
	struct groundtri *gp, gndpoly, *best_plane;
	double theta, best_dist, dist;
	struct vect v1, v2;
	struct pt p2;
	
	v1.z = 0;
	v2.z = 0;
	p2.x = p1->x;
	p2.y = p1->y;
	p2.z = 0;

	best_plane = NULL;
	best_dist = 0;
	best_flag = 0;

	for (gp = first_groundtri; gp; gp = gp->next) {
		theta = 0;

		gndpoly = *gp;

		gndpoly.p1.z = 0;
		gndpoly.p2.z = 0;
		gndpoly.p3.z = 0;

		psub (&v1, &p2, &gndpoly.p1);
		vnorm (&v1, &v1);

		psub (&v2, &p2, &gndpoly.p2);
		vnorm (&v2, &v2);

		theta += acos (vdot (&v1, &v2));


		psub (&v1, &p2, &gndpoly.p2);
		vnorm (&v1, &v1);

		psub (&v2, &p2, &gndpoly.p3);
		vnorm (&v2, &v2);

		theta += acos (vdot (&v1, &v2));

		
		psub (&v1, &p2, &gndpoly.p3);
		vnorm (&v1, &v1);

		psub (&v2, &p2, &gndpoly.p1);
		vnorm (&v2, &v2);

		theta += acos (vdot (&v1, &v2));

		if (theta > DTOR (359)) {
			dist = fabs (gp->a * p1->x + gp->b * p1->y
				     + gp->c * p1->z + gp->d)
				/ sqrt (square (gp->a) + square (gp->b)
					+ square (gp->c));
			
			if (best_flag == 0) {
				best_flag = 1;
				best_plane = gp;
				best_dist = dist;
			} else if (dist < best_dist) {
				printf ("new best plane found, overriding"
					" old\n");
				best_plane = gp;
				best_dist = dist;
			}
		}
	}

	if (best_flag)
		return (best_plane);

	printf ("ERROR! Could not detect which plane point was on!\n");
	return (NULL);
}

double
ground_height (struct pt *p1)
{
	int best_flag;
	double theta, best_diff, best_z;
	struct vect v1, v2;
	struct groundtri *gp, gndpoly;
	struct pt p2;

	v1.z = 0;
	v2.z = 0;

	p2.x = p1->x;
	p2.y = p1->y;
	p2.z = 0;

	best_z = 0;
	best_diff = 0;
	best_flag = 0;

	for (gp = first_groundtri; gp; gp = gp->next) {
		theta = 0;
		
		gndpoly = *gp;

		gndpoly.p1.z = 0;
		gndpoly.p2.z = 0;
		gndpoly.p3.z = 0;
		
		psub (&v1, &p2, &gndpoly.p1);
		vnorm (&v1, &v1);

		psub (&v2, &p2, &gndpoly.p2);
		vnorm (&v2, &v2);

		theta = acos (vdot (&v1, &v2));

		psub (&v1, &p2, &gndpoly.p2);
		vnorm (&v1, &v1);

		psub (&v2, &p2, &gndpoly.p3);
		vnorm (&v2, &v2);

		theta += acos (vdot (&v1, &v2));

		psub (&v1, &p2, &gndpoly.p3);
		vnorm (&v1, &v1);
		
		psub (&v2, &p2, &gndpoly.p1);
		vnorm (&v2, &v2);

		theta += acos (vdot (&v1, &v2));

		if (theta > DTOR (359)) {
			double z = (-gp->a * p2.x - gp->b * p2.y - gp->d)
				/ gp->c;
			double diff = fabs (z - p1->z);

			if (best_flag == 0) {
				best_flag = 1;
				best_z = z;
				best_diff = diff;
			} else if (diff < best_diff) {
				best_z = z;
				best_diff = diff;
			}
		}
	}
	
	if (best_flag)
		return (best_z);
	
	printf ("ERROR! Could not find ground, expect weirdness!\n");

	return (1);
}

void
process_input (void)
{
	SDL_Event event;
	int key;

	while (SDL_PollEvent (&event)) {
		key = event.key.keysym.sym;
		switch (event.type) {
		case SDL_QUIT:
			exit (0);
		case SDL_KEYDOWN:
			switch (key) {
			case SDLK_UP:
			case 'w':
				arrowkey[UP] = 1;
				break;
			case SDLK_DOWN:
			case 's':
				arrowkey[DOWN] = 1;
				break;
			case SDLK_LEFT:
			case 'a':
				arrowkey[LEFT] = 1;
				if (mousebutton[3] == 1 || mousebutton[2] == 1)
					arrowkey[FAKE_Q] = 1;
				break;
			case SDLK_RIGHT:
			case 'd':
				arrowkey[RIGHT] = 1;
				if (mousebutton[3] == 1 || mousebutton[2] == 1)
					arrowkey[FAKE_E] = 1;
				break;
			case 'q':
				arrowkey[Q] = 1;
				break;
			case 'e':
				arrowkey[E] = 1;
				break;
			case ' ':
				if (player.p.z <= ground_height (&player.p)
				    && player.movtype == GROUNDED) {
					player.p.z += 1e-6;
					player.vel.z = 25;
					player.movtype = FALLING;
					printf ("jumping\n");
				}
				break;
			case 'r':
				player.p.x = 0;
				player.p.y = 0;
				player.p.z = ground_height (&player.p);
				break;
			case 'u':
				player.p.z = ground_height (&player.p) + 50;
				break;
			case 'm':
				printf ("%8.3f %8.3f %8.3f\n", player.p.x,
					player.p.y, player.p.z);
				printf ("%g\n", RTOD (player.loc->theta));
				break;
			case 'p':
				if (paused == NO) {
					paused = YES;
				} else {
					paused = NO;
				}
				break;
			case 't':
				player.p.x = -200.937;
				player.p.y = 162.219;
				player.p.z = 99.729;
				break;
			case 'y':
				player.p.x = 63.293;
				player.p.y = 78.532;
				player.p.z = .203;
				break;
			case 'n':
				vset (&player.vel, 0, 0, 0);
				break;
			}
			break;
		case SDL_KEYUP:
			switch (key) {
			case SDLK_ESCAPE:
				exit (0);
			case SDLK_UP:
			case 'w':
				arrowkey[UP] = 0;
				break;
			case SDLK_DOWN:
			case 's':
				arrowkey[DOWN] = 0;
				break;
			case SDLK_LEFT:
			case 'a':
				arrowkey[LEFT] = 0;
				if (arrowkey[FAKE_Q])
					arrowkey[FAKE_Q] = 0;
				break;
			case SDLK_RIGHT:
			case 'd':
				arrowkey[RIGHT] = 0;
				if (arrowkey[FAKE_E])
					arrowkey[FAKE_E] = 0;
				break;
			case 'q':
				arrowkey[Q] = 0;
				break;
			case 'e':
				arrowkey[E] = 0;
				break;
			}
		case SDL_MOUSEBUTTONDOWN:
			mousebutton[event.button.button] = 1;
			switch (event.button.button) {
			case 1:
				leftclickdown ();
				break;
			case 2:
				middleclickdown ();
				break;
			case 3:
				rightclickdown ();
				break;
			case 4:
				zoomin ();
				break;
			case 5:
				zoomout ();
				break;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			mousebutton[event.button.button] = 0;
			switch (event.button.button) {
			case 1:
				leftclickup ();
				break;
			case 2:
				middleclickup ();
				break;
			case 3:
				rightclickup ();
				break;
			case 4:
				zoomin ();
				break;
			case 5:
				zoomout ();
				break;
			}
			break;
		case SDL_MOUSEMOTION:
			mouse_x = event.button.x;
			mouse_y = event.button.y;
			break;
		}
	}				    
}

void
process_mouse (void)
{
	double newphi;

	if (mousebutton[1] == 0 && mousebutton[2] == 0 && mousebutton[3] == 0) {
		playercamera.theta = player.theta + DTOR (180)
			+ playercamera.theta_difference;
	} else if (mousebutton[2] == 1 || mousebutton[3] == 1) {
		player.theta = player.theta - DTOR (.5 * (mouse_x - WIDTH / 2));
		playercamera.theta = player.theta + DTOR (180);
		playercamera.theta_difference = DTOR (0);
		newphi = playercamera.phi + DTOR (.25 * (mouse_y - HEIGHT / 2));
		if (newphi >= DTOR (-89.99) && newphi <= DTOR (89.99)) {
			playercamera.phi = newphi;
		}
		SDL_WarpMouse (WIDTH / 2, HEIGHT / 2);
	} else if (mousebutton[1]) {
		playercamera.theta = fmod (playercamera.theta
					   - DTOR (.5 * (mouse_x - WIDTH / 2)),
					   DTOR (360));
		playercamera.theta_difference
			= playercamera.theta - (player.theta + DTOR (180));
		newphi = playercamera.phi + DTOR (.25 * (mouse_y - HEIGHT / 2));
		if (newphi >= DTOR (-89.99) && newphi <= DTOR (89.99)) {
			playercamera.phi = newphi;
		}
		SDL_WarpMouse (WIDTH / 2, HEIGHT / 2);
	}
}

void
leftclickdown (void)
{
	oldmouse_x = mouse_x;
	oldmouse_y = mouse_y;
	SDL_WarpMouse (WIDTH / 2, HEIGHT / 2);

	if (mousebutton[3]) {
		middleclickdown ();
		mousebutton[2] = 1;
	}
}

void
middleclickdown (void)
{
	arrowkey[UP] = 1;
	oldmouse_x = mouse_x;
	oldmouse_y = mouse_y;
	player.theta = playercamera.theta + DTOR (180);
	SDL_WarpMouse (WIDTH / 2, HEIGHT / 2);
}

void
rightclickdown (void)
{
	oldmouse_x = mouse_x;
	oldmouse_y = mouse_y;
	player.theta = playercamera.theta + DTOR (180);
	SDL_WarpMouse (WIDTH / 2, HEIGHT / 2);

	if (mousebutton[1]) {
		middleclickdown ();
		mousebutton[2] = 1;
	}
	if (arrowkey[LEFT])
		arrowkey[FAKE_Q] = 1;
	if (arrowkey[RIGHT])
		arrowkey[FAKE_E] = 1;
}

void
leftclickup (void)
{
	SDL_WarpMouse (oldmouse_x, oldmouse_y);

	if (mousebutton[2]) {
		mousebutton[2] = 0;
		middleclickup ();
	}
}

void
middleclickup (void)
{
	arrowkey[UP] = 0;
	SDL_WarpMouse (oldmouse_x, oldmouse_y);
}

void
rightclickup (void)
{
	SDL_WarpMouse (oldmouse_x, oldmouse_y);

	if (mousebutton[2]) {
		mousebutton[2] = 0;
		middleclickup ();
	}

	if (arrowkey[FAKE_Q])
		arrowkey[FAKE_Q] = 0;
	if (arrowkey[FAKE_E])
		arrowkey[FAKE_E] = 0;
}

void
zoomin (void)
{
	double newcamdist;
	
	newcamdist = player.camdist - .1;
	
	if (newcamdist > 0)
		player.camdist = newcamdist;
}

void
zoomout (void)
{
	double newcamdist;

	newcamdist = player.camdist + .1;
	
	if (newcamdist <= 30)
		player.camdist = newcamdist;
}

void
movement (void)
{
	double now, dt;
	
	now = get_secs ();

	dt = now - player.lasttime;

	if (arrowkey[LEFT] != arrowkey[RIGHT] && mousebutton[3] == 0
	    && mousebutton[2] == 0) {
		if (arrowkey[LEFT]) {
			player.theta = player.theta + player.turnspeed * dt;
		}
		if (arrowkey[RIGHT]) {
			player.theta = player.theta - player.turnspeed * dt;
		}
		if (player.theta < 0) {
			player.theta = player.theta + DTOR (360);
		}
	}
	player.theta = fmod (player.theta, DTOR (360));
	
	player.moving = NO;
	
	if (arrowkey[UP] == 1)
		player.moving = FORW;
	if (arrowkey[DOWN] == 1)
		player.moving = BACK;
	if (arrowkey[Q] == 1 || arrowkey[FAKE_Q] == 1)
		player.moving = SIDE_Q;
	if (arrowkey[E] == 1 || arrowkey[FAKE_E] == 1)
		player.moving = SIDE_E;
	if ((arrowkey[Q] == 1 && arrowkey[UP] == 1)
	    || (arrowkey[FAKE_Q] == 1 && arrowkey[UP] == 1)
	    || (arrowkey[Q] == 1 && mousebutton[2] == 1)
	    || (arrowkey[FAKE_Q] == 1 && mousebutton[2] == 1))
		player.moving = FORW_Q;
	if ((arrowkey[E] == 1 && arrowkey[UP] == 1)
	    || (arrowkey[FAKE_E] == 1 && arrowkey[UP]  == 1)
	    || (arrowkey[E] == 1 && mousebutton[2] == 1)
	    || (arrowkey[FAKE_E] == 1 && mousebutton[2] == 1))
		player.moving = FORW_E;
	if ((arrowkey[Q] == 1 && arrowkey[DOWN] == 1)
	    || (arrowkey[FAKE_Q] == 1 && arrowkey[DOWN] == 1))
		player.moving = BACK_Q;
	if ((arrowkey[E] == 1 && arrowkey[DOWN] == 1)
	    || (arrowkey[FAKE_E] == 1 && arrowkey[DOWN] == 1))
		player.moving = BACK_E;
}

void
moving (void)
{
	struct groundtri *gp;

	printf ("moving function called\n");

	gp = detect_plane (&player.p);
	if ((player.p.z > ground_height (&player.p)
	     || gp->theta >= DTOR (50))
	    && player.movtype == GROUNDED) {
		player.movtype = FALLING;
		printf ("changing to falling straight from moving function\n");
	}

	player.loc = gp;

	printf ("player plane num: %d\n", player.loc->gndnum);

	switch (player.movtype) {
	case GROUNDED:
		grounded_moving ();
		break;
	case FALLING:
		falling_moving ();
		break;
	case FLYING:
		flying_moving ();
		break;
	}
}

void
grounded_moving (void)
{
	double now, dt;
	struct pt newpos;
	struct groundtri *gp;
	struct vect v1, v2, ctrl_vel;

	printf ("new grounded function called\n");

	now = get_secs ();
	dt = now - player.lasttime;

	ctrl_vel.x = 0;
	ctrl_vel.y = 0;
	ctrl_vel.z = 0;

	if (player.p.z <= ground_height (&player.p)) {
		printf ("player is on or under the ground, calculating how the "
			"user wants to move\n");
		switch (player.moving) {
		case BACK:
			ctrl_vel.x = -.3 * player.speed * cos (player.theta);
			ctrl_vel.y = -.3 * player.speed * sin (player.theta);
			break;
		case FORW:
			ctrl_vel.x = player.speed * cos (player.theta);
			ctrl_vel.y = player.speed * sin (player.theta);
			break;
		case SIDE_Q:
			ctrl_vel.x = player.speed
				* cos (player.theta + DTOR (90));
			ctrl_vel.y = player.speed
				* sin (player.theta + DTOR (90));
			break;
		case SIDE_E:
			ctrl_vel.x = player.speed
				* cos (player.theta - DTOR (90));
			ctrl_vel.y = player.speed
				* sin (player.theta - DTOR (90));
			break;
		case FORW_Q:
			ctrl_vel.x = player.speed
				* cos (player.theta + DTOR (45));
			ctrl_vel.y = player.speed
				* sin (player.theta + DTOR (45));
			break;
		case FORW_E:
			ctrl_vel.x = player.speed
				* cos (player.theta - DTOR (45));
			ctrl_vel.y = player.speed
				* sin (player.theta - DTOR (45));
			break;
		case BACK_Q:
			ctrl_vel.x = -.3 * player.speed
				* cos (player.theta - DTOR (45));
			ctrl_vel.y = -.3 * player.speed
				* sin (player.theta - DTOR (45));
			break;
		case BACK_E:
			ctrl_vel.x = -.3 * player.speed
				* cos (player.theta + DTOR (45));
			ctrl_vel.y = -.3 * player.speed
				* sin (player.theta + DTOR (45));
			break;
		}
	}

	newpos.x = player.p.x + ctrl_vel.x * dt;
	newpos.y = player.p.y + ctrl_vel.y * dt;
	newpos.z = player.p.z + ctrl_vel.z * dt;

	psub (&player.vel, &newpos, &player.p);
	vscal (&player.vel, &player.vel, 1 / dt);

	gp = detect_plane (&newpos);

	if (gp->theta < DTOR (50)) {
		printf ("plane shallow enough\n");
		newpos.z = ground_height (&newpos);
		psub (&player.vel, &newpos, &player.p);
		vscal (&player.vel, &player.vel, 1 / dt);
		player.p = newpos;
	} else if (newpos.z > ground_height (&newpos)) {
		printf ("ground dropping away, setting to falling\n");
		player.p = newpos;
		player.movtype = FALLING;
	} else if (newpos.z <= ground_height (&newpos)) {
		printf ("ground steeply up, sliding along base\n");
		if (gp == player.loc) {
			printf ("line %d: odd bug, exiting\n", __LINE__);
			exit (1);
		}
		vnorm (&v2, &ctrl_vel);
		vcross (&v1, &player.loc->normv, &gp->normv);
		vnorm (&v1, &v1);
		if (vdot (&v1, &v2) < 0)
			vscal (&v1, &v1, -1);
		vscal (&ctrl_vel, &v1, vdot (&ctrl_vel, &v1));

		newpos.x = player.p.x + ctrl_vel.x * dt;
		newpos.y = player.p.y + ctrl_vel.y * dt;
		newpos.z = player.p.z + ctrl_vel.z * dt;

		if ((gp = detect_plane (&newpos)) == NULL) {
			printf ("can't detect plane, exiting\n");
			exit (1);
		}

		printf ("newpos on %d\n", gp->gndnum);

		if (gp->theta < DTOR (50)) {
			printf ("sliding along edge of steep slope while "
				"moving up shallow slope\n");
			psub (&player.vel, &newpos, &player.p);
			vscal (&player.vel, &player.vel, 1 / dt);

			player.p = newpos;
		} else {
			printf ("blah\n");
		}
	} else {
		printf ("this should never be printed");
	}
}

void
falling_moving (void)
{
	double now, dt;
	struct vect grav, grav_acc;
	struct groundtri *gp;
	struct pt newpos, p1;

	printf ("new falling function called\n");

	now = get_secs ();
	dt = now - player.lasttime;

	if ((gp = detect_plane (&player.p)) == NULL) {
		printf ("Can't find ground, moving player to start position "
			"to avoid seg fault (in new falling function)\n");
		player.p.x = 0;
		player.p.y = 0;
		player.p.z = ground_height (&player.p);
		player.movtype = GROUNDED;
		player.loc = detect_plane (&player.p);
		return;
	}

	if (player.p.z <= ground_height (&player.p) && gp->theta < DTOR (50)) {
		printf ("changing to grounded from falling function, "
			"player is underground and plane is shallow enough");
		player.movtype = GROUNDED;
		player.p.z = ground_height (&player.p);
		return;
	}

	printf ("old position checks out, applying grav to newpos and "
		"checking newpos\n");

	grav.x = 0;
	grav.y = 0;
	grav.z = GRAVITY;

	vscal (&grav_acc, &grav, 1 / player.mass);

	player.vel.x += grav_acc.x;
	player.vel.y += grav_acc.y;
	player.vel.z += grav_acc.z;

	newpos.x = player.p.x + player.vel.x * dt;
	newpos.y = player.p.y + player.vel.y * dt;
	newpos.z = player.p.z + player.vel.z * dt;

	if (newpos.z <= ground_height (&newpos)) {
		if ((gp = detect_plane (&newpos)) == NULL) {
			printf ("Can't find ground at newpos, moving player to "
				"start position to avoid seg fault\n");
			player.p.x = 0;
			player.p.y = 0;
			player.p.z = ground_height (&player.p);
			player.movtype = GROUNDED;
			player.loc = detect_plane (&player.p);
			return;
		}

		if (gp->theta < DTOR (50)) {
			printf ("changing to grounded, player fell underground "
				"on a shallow slope\n");
			player.p = newpos;
			player.p.z = ground_height (&player.p);
			player.movtype = GROUNDED;
			player.loc = gp;
			return;
		} else {
			printf ("sliding down steep plane\n");
			p1.x = vdot (&player.vel, &gp->normv) * gp->normv.x;
			p1.y = vdot (&player.vel, &gp->normv) * gp->normv.y;
			p1.z = vdot (&player.vel, &gp->normv) * gp->normv.z;

			player.vel.x -= p1.x;
			player.vel.y -= p1.y;
			player.vel.z -= p1.z;
			
			player.p.x += player.vel.x * dt;
			player.p.y += player.vel.y * dt;
			player.p.z += player.vel.z * dt;
		}
	} else {
		printf ("normal falling\n");

		player.p.x = newpos.x;
		player.p.y = newpos.y;
		player.p.z = newpos.z;
		
		player.loc = gp;
	}
}

void
flying_moving (void)
{
	printf ("flying\n");
}

void
draw (void)
{
	GLfloat light0_diffuse[] = {1, 1, 1, 0};
	GLfloat light0_position[] = {1, 1, 1, 0};
	GLfloat light1_ambient[] = {.5, .5, .5, 0};
	GLfloat light1_position[] = {0, 0, 1, 0};
	GLfloat light2_ambient[] = {.1, .1, .1, 0};
	GLfloat light2_position[] = {100, 100, 100, 0};

	glLoadIdentity ();
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	distanceLookAt (player.p.x, player.p.y, player.p.z + .7, player.camdist,
			playercamera.theta, playercamera.phi);

	color_coords (1);
	
	glDisable (GL_LIGHTING);
	glBegin (GL_LINES);
	glColor3f (1, 0, 0);
	glVertex3f (player.p.x, player.p.y, player.p.z + 1);
	glVertex3f (player.p.x + player.vel.x * 10,
		    player.p.y + player.vel.y * 10,
		    player.p.z + player.vel.z * 10 + 1);
	glEnd ();

	glLightfv (GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv (GL_LIGHT0, GL_POSITION, light0_position);
	glLightfv (GL_LIGHT1, GL_AMBIENT, light1_ambient);
	glLightfv (GL_LIGHT1, GL_POSITION, light1_position);
	glLightfv (GL_LIGHT2, GL_AMBIENT, light2_ambient);
	glLightfv (GL_LIGHT2, GL_POSITION, light2_position);

	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT1);
	glEnable (GL_LIGHT2);

	glCallList (terrain_list);

	glDisable (GL_LIGHT2);

	glPushMatrix ();
	glTranslatef (player.p.x, player.p.y, player.p.z + .7);
	
	glRotatef (RTOD (player.theta), 0, 0, 1);
	glRotatef (90, 1, 0, 0);

	glEnable (GL_LIGHT0);
	glutSolidTeapot (1);
	glDisable (GL_LIGHT0);

	glDisable (GL_LIGHTING);

	glFlush ();

	SDL_GL_SwapBuffers ();
}

int
main (int argc, char **argv)
{
	int i;
	double now;
	
		
	feenableexcept(FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);


	init_sdl_gl_flags (WIDTH, HEIGHT, 0);

	srandom (time (NULL));

	init_gl (&argc, argv);

	glEnable (GL_LIGHTING);
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_AUTO_NORMAL);
	glEnable (GL_NORMALIZE);

	glClearDepth (1);
	
	glViewport (0, 0, WIDTH, HEIGHT);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (60, (GLfloat) WIDTH/(GLfloat) HEIGHT, .1, 1000);
	glClearColor (0, 0, 0, 0);
	glMatrixMode (GL_MODELVIEW);

	SDL_ShowCursor (1);

	makeImages ();

	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
	glGenTextures (1, texName);
	
	makeTexture (texName[0], groundtexture.texturesize,
		     (GLubyte ***) groundtexture.tex);

	glTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE);

	gndcounter = 1;
	read_terrain ();

	player.p.x = 0;
	player.p.y = 0;
	player.p.z = ground_height (&player.p);

	player.loc = detect_plane (&player.p);

	player.movtype = GROUNDED;

	player.speed = 100;
	player.mass = 1;

	vset (&player.vel, 0, 0, 0);

	player.turnspeed = DTOR (180);
	player.theta = DTOR (0);
	player.camdist = 15;

	player.lasttime = get_secs ();
	player.moving = NO;

	playercamera.phi = DTOR (0);
	playercamera.theta_difference = 0;
	
	while (1) {
		process_input ();
		
		if (mousebutton[1] == 0
		    && mousebutton[2] == 0
		    && mousebutton[3] == 0) {
			SDL_ShowCursor (1);
		} else {
			SDL_ShowCursor (0);
		}

		movement ();
		if (paused == NO) {
			for (i = 0; i < 1; i++) {
				moving ();
				now = get_secs ();
				player.lasttime = now;
			}
		}

		process_mouse ();

		draw ();
		
		now = get_secs ();
		
		player.lasttime = now;

		SDL_Delay (10);
	}

	return (0);
}
