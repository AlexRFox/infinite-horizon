#include "alex.h"

enum {
	WIDTH = 800,
	HEIGHT = 600,

	UP = 1,
	DOWN = 2,
	RIGHT = 3,
	LEFT = 4,

	FORW = 1,
	BACK = 2,
	SIDE_Q = 3,
	SIDE_E = 4,
	FORW_Q = 5,
	FORW_E = 6,
	BACK_Q = 7,
	BACK_E = 8,

	YES = 1,
	NO = 0,

	Q = 1,
	E = 2,
	SPACE = 3,
	FAKE_Q = 4,
	FAKE_E = 5,

	GRAVITY = -50,
	FRICTION = 1,
	PLAYERMAXVEL = 100,

	GROUNDED = 0,
	FALLING = 1,
	FLYING = 2,
	SLIDING = 3,
};

#define GROUNDTHRESHOLD .1

GLuint texName[3];

int n;

int mousebutton[8], arrowkey[8], wasd[8], misckey[64];
double mouse_x, mouse_y, oldmouse_x, oldmouse_y;
GLuint terrain_list;

struct unit {
	struct pt p;
	double theta, vel_max, turnspeed, camdist;
	double mov_vel_x, mov_vel_y, mov_vel_z, mass, force;
	double lasttimemov, lasttimeturn;
	int movtype, moving;
};

struct unit player;

struct groundtri {
	struct groundtri *next;
	double a, b, c, normtheta, theta;
	struct pt p1, p2, p3, p4;
	struct vect normv, grad;
};

struct groundtri *first_groundtri, *last_groundtri;

struct groundtexture {
	int texturesize;
	GLubyte *tex;
};

struct groundtexture groundtexture;

struct groundtri *player_plane;

struct groundtri *
detect_player_plane (struct pt p)
{
	struct groundtri *gp, gndpoly;
	double theta;
	struct vect v1, v2;

	v1.z = 0;
	v2.z = 0;
	p.z = 0;

	for (gp = first_groundtri; gp; gp = gp->next) {
		theta = 0;

		gndpoly = *gp;

		gndpoly.p1.z = 0;
		gndpoly.p2.z = 0;
		gndpoly.p3.z = 0;

		psub (&v1, &p, &gndpoly.p1);
		vnorm (&v1, &v1);

		psub (&v2, &p, &gndpoly.p2);
		vnorm (&v2, &v2);

		theta += acos (vdot (&v1, &v2));


		psub (&v1, &p, &gndpoly.p2);
		vnorm (&v1, &v1);

		psub (&v2, &p, &gndpoly.p3);
		vnorm (&v2, &v2);

		theta += acos (vdot (&v1, &v2));

		
		psub (&v1, &p, &gndpoly.p3);
		vnorm (&v1, &v1);

		psub (&v2, &p, &gndpoly.p1);
		vnorm (&v2, &v2);

		theta += acos (vdot (&v1, &v2));

		if (theta > DTOR (359)) {
			return (gp);
		}
	}

	printf ("ERROR! Could not detect which plane point was on!\n");
	return (NULL);
}

double
check_collision (struct pt *p1, struct pt *p2, struct groundtri *gp)
{
	struct vect old_vect, new_vect, v1, v2, v3, v4, norm_vect;
	double old_dp, new_dp, a, b, c, t, theta;
	struct pt p3;

	theta = 0;

	psub (&old_vect, p1, &gp->p1);

	psub (&new_vect, p2, &gp->p1);

	psub (&v1, &gp->p1, &gp->p2);

	psub (&v2, &gp->p1, &gp->p3);

	vcross (&norm_vect, &v1, &v2);

	old_dp = vdot (&old_vect, &norm_vect);

	new_dp = vdot (&new_vect, &norm_vect);

	a = p1->x - p2->x;
	b = p1->y - p2->y;
	c = p1->z - p2->z;

	t = ((gp->a * p1->x) + (gp->b * p1->y) + gp->c - p1->z)
		/ (c - (gp->a * a) - (gp->b * b));

	p3.x = p1->x + (a * t);
	p3.y = p1->y + (b * t);
	p3.z = p1->z + (c * t);

	psub (&v3, &p3, &gp->p1);
	vnorm (&v3, &v3);

	psub (&v4, &p3, &gp->p2);
	vnorm (&v4, &v4);

	theta += acos (vdot (&v3, &v4));


	psub (&v3, &p3, &gp->p2);
	vnorm (&v3, &v3);

	psub (&v4, &p3, &gp->p3);
	vnorm (&v4, &v4);

	theta += acos (vdot (&v3, &v4));


	psub (&v3, &p3, &gp->p3);
	vnorm (&v3, &v3);

	psub (&v4, &p3, &gp->p4);
	vnorm (&v4, &v4);

	theta += acos (vdot (&v3, &v4));


	psub (&v3, &p3, &gp->p4);
	vnorm (&v3, &v3);
	
	psub (&v4, &p3, &gp->p1);
	vnorm (&v4, &v4);

	theta += acos (vdot (&v3, &v4));

	if (theta >= DTOR (359))
		return (t);

	return (0);
}

void
makeImages ()
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
		fgets (s, sizeof s, f);
		if (s[0] == '#')
			i--;
		if (i == 1) {
			strtok (s, " ");
			groundtexture.texturesize = atof (s);
		}
	}

	groundtexture.tex
		= xcalloc (groundtexture.texturesize * groundtexture.texturesize * 4,
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

double
ground_height (struct pt p)
{
	int best_flag;
	double theta, best_diff, best_z;
	struct vect v1, v2;
	struct groundtri *gp, gndpoly;

	v1.z = 0;
	v2.z = 0;
	p.z = 0;

	for (gp = first_groundtri; gp; gp = gp->next) {
		theta = 0;

		gndpoly = *gp;

		gndpoly.p1.z = 0;
		gndpoly.p2.z = 0;
		gndpoly.p3.z = 0;

		psub (&v1, &p, &gndpoly.p1);
		vnorm (&v1, &v1);

		psub (&v2, &p, &gndpoly.p2);
		vnorm (&v2, &v2);

		theta += acos (vdot (&v1, &v2));


		psub (&v1, &p, &gndpoly.p2);
		vnorm (&v1, &v1);

		psub (&v2, &p, &gndpoly.p3);
		vnorm (&v2, &v2);

		theta += acos (vdot (&v1, &v2));

		
		psub (&v1, &p, &gndpoly.p3);
		vnorm (&v1, &v1);

		psub (&v2, &p, &gndpoly.p1);
		vnorm (&v2, &v2);

		theta += acos (vdot (&v1, &v2));

		if (theta > DTOR (359)) {
			double z = gp->a * p.x + gp->b * p.y + gp->c;
			double diff = fabs (z - p.z);

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
zoomin (void)
{
	double newcamdist;

	newcamdist = player.camdist -.1;

	if (newcamdist > 0)
		player.camdist = player.camdist - .1;
}

void
zoomout (void)
{
	double newcamdist;

	newcamdist = player.camdist + .1;

	if (newcamdist <= 15)
		player.camdist = player.camdist + .1;
}

void
movement (void)
{
	double now, dt;
	
	now = get_secs ();

	dt = now - player.lasttimeturn;

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
	if (misckey[Q] == 1 || misckey[FAKE_Q] == 1)
		player.moving = SIDE_Q;
	if (misckey[E] == 1 || misckey[FAKE_E] == 1)
		player.moving = SIDE_E;
	if ((misckey[Q] == 1 && arrowkey[UP] == 1)
	    || (misckey[FAKE_Q] == 1 && arrowkey[UP] == 1)
	    || (misckey[Q] == 1 && mousebutton[2] == 1)
	    || (misckey[FAKE_Q] == 1 && mousebutton[2] == 1))
		player.moving = FORW_Q;
	if ((misckey[E] == 1 && arrowkey[UP] == 1)
	    || (misckey[FAKE_E] == 1 && arrowkey[UP]  == 1)
	    || (misckey[E] == 1 && mousebutton[2] == 1)
	    || (misckey[FAKE_E] == 1 && mousebutton[2] == 1))
		player.moving = FORW_E;
	if ((misckey[Q] == 1 && arrowkey[DOWN] == 1)
	    || (misckey[FAKE_Q] == 1 && arrowkey[DOWN] == 1))
		player.moving = BACK_Q;
	if ((misckey[E] == 1 && arrowkey[DOWN] == 1)
	    || (misckey[FAKE_E] == 1 && arrowkey[DOWN] == 1))
		player.moving = BACK_E;
}

void
grounded_moving (void)
{
	double now, dt, acc_x, acc_y, mov_hypot;
	struct pt player_newpos;

	now = get_secs ();

	dt = now - player.lasttimemov;

	switch (player.moving) {
	case NO:
		player.mov_vel_x = 0;
		player.mov_vel_y = 0;
		player.mov_vel_z = 0;
		break;
	case BACK:
		acc_x = (player.force / player.mass) * cos (player.theta) * dt;
		acc_y = (player.force / player.mass) * sin (player.theta) * dt;
		
		player.mov_vel_x -= acc_x;
		player.mov_vel_y -= acc_y;
		
		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);
		
		if (mov_hypot > .3 * PLAYERMAXVEL) {
			player.mov_vel_x *= ((.3 * PLAYERMAXVEL) / mov_hypot);
			player.mov_vel_y *= ((.3 * PLAYERMAXVEL) / mov_hypot);
		}
		break;
	case FORW:
		acc_x = (player.force / player.mass) * cos (player.theta) * dt;
		acc_y = (player.force / player.mass) * sin (player.theta) * dt;
		
		player.mov_vel_x += acc_x;
		player.mov_vel_y += acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > PLAYERMAXVEL) {
			player.mov_vel_x *= (PLAYERMAXVEL / mov_hypot);
			player.mov_vel_y *= (PLAYERMAXVEL / mov_hypot);
		}
		break;
	case SIDE_Q:
		acc_x = (player.force / player.mass)
			* cos (player.theta + DTOR (90)) * dt;
		acc_y = (player.force / player.mass)
			* sin (player.theta + DTOR (90)) * dt;
		
		player.mov_vel_x += acc_x;
		player.mov_vel_y += acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > PLAYERMAXVEL) {
			player.mov_vel_x *= (PLAYERMAXVEL / mov_hypot);
			player.mov_vel_y *= (PLAYERMAXVEL / mov_hypot);
		}
		break;
	case SIDE_E:
		acc_x = (player.force / player.mass)
			* cos (player.theta - DTOR (90)) * dt;
		acc_y = (player.force / player.mass)
			* sin (player.theta - DTOR (90)) * dt;
		
		player.mov_vel_x += acc_x;
		player.mov_vel_y += acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > PLAYERMAXVEL) {
			player.mov_vel_x *= (PLAYERMAXVEL / mov_hypot);
			player.mov_vel_y *= (PLAYERMAXVEL / mov_hypot);
		}
		break;
	case FORW_Q:
		acc_x = (player.force / player.mass)
			* cos (player.theta + DTOR (45)) * dt;
		acc_y = (player.force / player.mass)
			* sin (player.theta + DTOR (45)) * dt;
		
		player.mov_vel_x += acc_x;
		player.mov_vel_y += acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > PLAYERMAXVEL) {
			player.mov_vel_x *= (PLAYERMAXVEL / mov_hypot);
			player.mov_vel_y *= (PLAYERMAXVEL / mov_hypot);
		}
		break;
	case FORW_E:
		acc_x = (player.force / player.mass)
			* cos (player.theta - DTOR (45)) * dt;
		acc_y = (player.force / player.mass)
			* sin (player.theta - DTOR (45)) * dt;
		
		player.mov_vel_x += acc_x;
		player.mov_vel_y += acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > PLAYERMAXVEL) {
			player.mov_vel_x *= (PLAYERMAXVEL / mov_hypot);
			player.mov_vel_y *= (PLAYERMAXVEL / mov_hypot);
		}
		break;
	case BACK_Q:
		acc_x = (player.force / player.mass)
			* cos (player.theta - DTOR (45)) * dt;
		acc_y = (player.force / player.mass)
			* sin (player.theta - DTOR (45)) * dt;
		
		player.mov_vel_x -= acc_x;
		player.mov_vel_y -= acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > (.3 * PLAYERMAXVEL)) {
			player.mov_vel_x *= ((.3 * PLAYERMAXVEL) / mov_hypot);
			player.mov_vel_y *= ((.3 * PLAYERMAXVEL) / mov_hypot);
		}
		break;
	case BACK_E:
		acc_x = (player.force / player.mass)
			* cos (player.theta + DTOR (45)) * dt;
		acc_y = (player.force / player.mass)
			* sin (player.theta + DTOR (45)) * dt;
		
		player.mov_vel_x -= acc_x;
		player.mov_vel_y -= acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > (.3 * PLAYERMAXVEL)) {
			player.mov_vel_x *= ((.3 * PLAYERMAXVEL) / mov_hypot);
			player.mov_vel_y *= ((.3 * PLAYERMAXVEL) / mov_hypot);
		}
		break;
	}

	player_newpos.x = player.p.x + player.mov_vel_x * dt;
	player_newpos.y = player.p.y + player.mov_vel_y * dt;
	player_newpos.z = ground_height (player_newpos);

	detect_player_plane (player_newpos);

	if (player_plane->theta > DTOR (50)) {
		player.movtype = FALLING;
		player_newpos.z = player.p.z;
	}

	player.p = player_newpos;

	printf ("GROUNDED\n");
}

void
falling_moving (void)
{
	printf ("FALLING\n");
	double now, dt, gndheight;
	struct pt player_newpos;

	now = get_secs ();

	dt = now - player.lasttimemov;

	player.mov_vel_z -= GRAVITY * dt;

	player_newpos.x = player.p.x + player.mov_vel_x * dt;
	player_newpos.y = player.p.y + player.mov_vel_y * dt;
	player_newpos.z = player.p.z + player.mov_vel_z * dt;

	gndheight = ground_height (player_newpos);

	if (player_newpos.z < gndheight) {
		player.movtype = GROUNDED;
		player_newpos.z = gndheight;
		player.mov_vel_z = 0;
	}

	player.p = player_newpos;
}

void
flying_moving (void)
{
	printf ("FLYING MOVING\n");
}

void
sliding_falling (void)
{
	printf ("SLIDING FALLING\n");
}

void
moving (void)
{
	if (player.mov_vel_z != 0)
		player.movtype = FALLING;

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
	case SLIDING:
		//sliding_moving ();
		break;
	}

	/*	double now, dt, acc_x, acc_y, mov_hypot, ground_z, grounded,
		t[16], j, l, m, n, a, b, c;
	int i, k;
	struct groundtri *gp;
	struct pt player_newpos;

	now = get_secs ();

	dt = now - player.lasttimemov;
 
	ground_z = ground_collision_height (player.p);

	if (fabs (ground_z - player.p.z) <= GROUNDTHRESHOLD) {
		player.p.z = ground_z;
		player.mov_vel_x = 0;
		player.mov_vel_y = 0;
		
		grounded = YES;
	} else {
		grounded = NO;
	}

	if (player.moving == BACK && grounded == YES) {
		acc_x = (player.force / player.mass) * cos (player.theta) * dt;
		acc_y = (player.force / player.mass) * sin (player.theta) * dt;
		
		player.mov_vel_x -= acc_x;
		player.mov_vel_y -= acc_y;
		
		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);
		
		if (mov_hypot > .3 * PLAYERMAXVEL) {
			player.mov_vel_x *= ((.3 * PLAYERMAXVEL) / mov_hypot);
			player.mov_vel_y *= ((.3 * PLAYERMAXVEL) / mov_hypot);
		}
	} else if (player.moving == FORW && grounded == YES) {
		acc_x = (player.force / player.mass) * cos (player.theta) * dt;
		acc_y = (player.force / player.mass) * sin (player.theta) * dt;
		
		player.mov_vel_x += acc_x;
		player.mov_vel_y += acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > PLAYERMAXVEL) {
			player.mov_vel_x *= (PLAYERMAXVEL / mov_hypot);
			player.mov_vel_y *= (PLAYERMAXVEL / mov_hypot);
		}
	} else if (player.moving == SIDE_Q && grounded == YES) {
		acc_x = (player.force / player.mass)
			* cos (player.theta + DTOR (90)) * dt;
		acc_y = (player.force / player.mass)
			* sin (player.theta + DTOR (90)) * dt;
		
		player.mov_vel_x += acc_x;
		player.mov_vel_y += acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > PLAYERMAXVEL) {
			player.mov_vel_x *= (PLAYERMAXVEL / mov_hypot);
			player.mov_vel_y *= (PLAYERMAXVEL / mov_hypot);
		}		
	} else if (player.moving == SIDE_E && grounded == YES) {
		acc_x = (player.force / player.mass)
			* cos (player.theta - DTOR (90)) * dt;
		acc_y = (player.force / player.mass)
			* sin (player.theta - DTOR (90)) * dt;
		
		player.mov_vel_x += acc_x;
		player.mov_vel_y += acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > PLAYERMAXVEL) {
			player.mov_vel_x *= (PLAYERMAXVEL / mov_hypot);
			player.mov_vel_y *= (PLAYERMAXVEL / mov_hypot);
		}
	} else if (player.moving == FORW_Q && grounded == YES) {
		acc_x = (player.force / player.mass)
			* cos (player.theta + DTOR (45)) * dt;
		acc_y = (player.force / player.mass)
			* sin (player.theta + DTOR (45)) * dt;
		
		player.mov_vel_x += acc_x;
		player.mov_vel_y += acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > PLAYERMAXVEL) {
			player.mov_vel_x *= (PLAYERMAXVEL / mov_hypot);
			player.mov_vel_y *= (PLAYERMAXVEL / mov_hypot);
		}
	} else if (player.moving == FORW_E && grounded == YES) {
		acc_x = (player.force / player.mass)
			* cos (player.theta - DTOR (45)) * dt;
		acc_y = (player.force / player.mass)
			* sin (player.theta - DTOR (45)) * dt;
		
		player.mov_vel_x += acc_x;
		player.mov_vel_y += acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > PLAYERMAXVEL) {
			player.mov_vel_x *= (PLAYERMAXVEL / mov_hypot);
			player.mov_vel_y *= (PLAYERMAXVEL / mov_hypot);
		}
	} else if (player.moving == BACK_Q && grounded == YES) {
		acc_x = (player.force / player.mass)
			* cos (player.theta - DTOR (45)) * dt;
		acc_y = (player.force / player.mass)
			* sin (player.theta - DTOR (45)) * dt;
		
		player.mov_vel_x -= acc_x;
		player.mov_vel_y -= acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > (.3 * PLAYERMAXVEL)) {
			player.mov_vel_x *= ((.3 * PLAYERMAXVEL) / mov_hypot);
			player.mov_vel_y *= ((.3 * PLAYERMAXVEL) / mov_hypot);
		}
	} else if (player.moving == BACK_E && grounded == YES) {
		acc_x = (player.force / player.mass)
			* cos (player.theta + DTOR (45)) * dt;
		acc_y = (player.force / player.mass)
			* sin (player.theta + DTOR (45)) * dt;
		
		player.mov_vel_x -= acc_x;
		player.mov_vel_y -= acc_y;

		mov_hypot = hypot (player.mov_vel_x, player.mov_vel_y);

		if (mov_hypot > (.3 * PLAYERMAXVEL)) {
			player.mov_vel_x *= ((.3 * PLAYERMAXVEL) / mov_hypot);
			player.mov_vel_y *= ((.3 * PLAYERMAXVEL) / mov_hypot);
		}
	}

	if (grounded == NO) {
		player.mov_vel_z -= GRAVITY * dt;
	}

	i = 0;

	player_newpos.x = player.p.x + player.mov_vel_x * dt;
	player_newpos.y = player.p.y + player.mov_vel_y * dt;
	player_newpos.z = player.p.z + player.mov_vel_z * dt;

	for (gp = first_groundtri; gp; gp = gp->next) {
		j = check_collision (&player.p, &player_newpos, gp);
		if (j != 0) {
			t[i] = j;
			i++;
		}
	}

	l = 0;

	for (k = 1; k <= i; k++) {
		if ((m =
		    abs (pow (player.p.x - player.p.x + player.mov_vel_x * dt, 2)
			 + pow (player.p.y - player.p.y + player.mov_vel_y * dt, 2)
			 + pow (player.p.z - player.p.z + player.mov_vel_z * dt,
				2)))
		    > l) {
			l = m;
			n = t[k];
		}
	}

	a = player.p.x - (player.p.x + player.mov_vel_x * dt);
	b = player.p.y - (player.p.y + player.mov_vel_y * dt);
	c = player.p.z - (player.p.z + player.mov_vel_z * dt);

	if (l == 0) {
		player.p.x += player.mov_vel_x * dt;
		player.p.y += player.mov_vel_y * dt;
		player.p.z += player.mov_vel_z * dt;
	} else {
		player.p.x += (a * n);
		player.p.y += (b * n);
		player.p.z += (c * n);
	}

	player.p.z = ground_collision_height (player.p);*/
}

void
new_moving (void)
{
	double ground_z, now, dt, fz, az, ground_stiffness,
		ground_damping, fx, forw_thrust, ax, friction, fy, ay,
		theta, new_moving_height;
	struct groundtri *gp;
	struct vect v1, v2, v3, v4, grad, grav, ctrl_force;

	now = get_secs ();
	dt = now - player.lasttimemov;

	ground_z = ground_height (player.p);

	forw_thrust = 10000;
	friction = 50;
	ground_stiffness = 7500;
	ground_damping = 100;

	zerovect (&ctrl_force);

	fx = 0;
	fy = 0;
	fz = 0;

	grav.x = 0;
	grav.y = 0;
	grav.z = GRAVITY * player.mass;

	zerovect (&v1);
	zerovect (&v2);
	zerovect (&v3);
	zerovect (&v4);

	new_moving_height = player.p.z - ground_z;

	/*	if (arrowkey[UP] && new_moving_height <= 0) {
		ctrl_force.x += forw_thrust * cos (player.theta);
		ctrl_force.y += forw_thrust * sin (player.theta);
		}*/
	if (new_moving_height <= 0) {
		switch (player.moving) {
		case BACK:
			ctrl_force.x = -.3 * forw_thrust * cos (player.theta);
			ctrl_force.y = -.3 * forw_thrust * sin (player.theta);
			break;
		case FORW:
			ctrl_force.x = forw_thrust * cos (player.theta);
			ctrl_force.y = forw_thrust * sin (player.theta);
			break;
		case SIDE_Q:
			ctrl_force.x = forw_thrust
				* cos (player.theta + DTOR (90));
			ctrl_force.y = forw_thrust
				* sin (player.theta + DTOR (90));
			break;
		case SIDE_E:
			ctrl_force.x = forw_thrust
				* cos (player.theta - DTOR (90));
			ctrl_force.y = forw_thrust
				* sin (player.theta - DTOR (90));
			break;
		case FORW_Q:
			ctrl_force.x = forw_thrust
				* cos (player.theta + DTOR (45));
			ctrl_force.y = forw_thrust
				* sin (player.theta + DTOR (45));
			break;
		case FORW_E:
			ctrl_force.x = forw_thrust
				* cos (player.theta - DTOR (45));
			ctrl_force.y = forw_thrust
				* sin (player.theta - DTOR (45));
			break;
		case BACK_Q:
			ctrl_force.x = -.3 * forw_thrust
				* cos (player.theta - DTOR (45));
			ctrl_force.y = -.3 * forw_thrust
				* sin (player.theta - DTOR (45));
			break;
		case BACK_E:
			ctrl_force.x = -.3 * forw_thrust
				* cos (player.theta + DTOR (45));
			ctrl_force.y = -.3 * forw_thrust
				* sin (player.theta + DTOR (45));
			break;
		}
	}

	if (new_moving_height <= 0) {
		player.p.z = ground_height (player.p);
		player.mov_vel_z = 0;

		if ((gp = detect_player_plane (player.p)) == NULL) {
			printf ("Can't find ground, moving player"
				" to start pos\n");
			player.p.x = 0;
			player.p.y = 0;
			player.p.z = ground_height (player.p);
			gp = detect_player_plane (player.p);
		}

		vnorm (&v1, &grav);
		theta = acos (vdot (&gp->grad, &v1));
		
		vinvert (&grad, &gp->grad);

		vscalmul (&v2, &grad, hypot3v (&grav) * sin (theta));
		vscalmul (&v3, &grad, hypot3v (&grav) * cos (theta));
		
		vinvert (&v4, &v3);
	} else {
		v2 = grav;
	}

	fx = ctrl_force.x + v2.x + v3.x + v4.x;
	fy = ctrl_force.y + v2.y + v3.y + v4.y;
	fz = ctrl_force.z + v2.z + v3.z + v4.z;

	ax = fx / player.mass;
	ay = fy / player.mass;
	az = fz / player.mass;

	player.mov_vel_x += ax * dt;
	player.mov_vel_y += ay * dt;
	player.mov_vel_z += az * dt;

	if (new_moving_height <= 0) {
		fx = -player.mov_vel_x * friction;
		fy = -player.mov_vel_y * friction;
		
		ax = (fx + v2.x + v3.x + v4.x) / player.mass;
		ay = (fy + v2.y + v3.y + v4.y) / player.mass;

		player.mov_vel_x += ax * dt;
		player.mov_vel_y += ay * dt;
	}

	player.p.x += player.mov_vel_x * dt;
	player.p.y += player.mov_vel_y * dt;
	player.p.z += player.mov_vel_z * dt;
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
define_plane (double x1, double y1, double z1,
	      double x2, double y2, double z2,
	      double x3, double y3, double z3,
	      double theta, struct vect *v1)
{
	struct groundtri *gp;

	gp = xcalloc (1, sizeof *gp);

	if (first_groundtri == NULL) {
		first_groundtri = gp;
	} else {
		last_groundtri->next = gp;
	}

	last_groundtri = gp;
	
	gp->a = (((y1 - y2) * z3)
		 / (x2 * y3 + x1 * (y2 - y3) - x3 * y2 + (x3 - x2) * y1))
		+ (((y3 - y1) * z2)
		   / (x2 * y3 + x1 * (y2 - y3) - x3 * y2 + (x3 - x2) * y1))
		+ (((y2 - y3) * z1)
		   / (x2 * y3 + x1 * (y2 - y3) - x3 * y2 + (x3 - x2) * y1));

	gp->b = (((x2 - x1) * z3)
		 / (x2 * y3 + x1 * (y2 - y3) - x3 * y2 + (x3 - x2) * y1))
		+ (((x1 - x3) * z2)
		   / (x2 * y3 + x1 * (y2 - y3) - x3 * y2 + (x3 - x2) * y1))
		+ (((x3 - x2) * z1)
		   / (x2 * y3 + x1 * (y2 - y3) - x3 * y2 + (x3 - x2) * y1));

	gp->c = (((x1 * y2 - x2 * y1) * z3)
		 / (x2 * y3 + x1 * (y2 - y3) - x3 * y2 + (x3 - x2) * y1))
		+ (((x3 * y1 - x1 * y3) * z2)
		   / (x2 * y3 + x1 * (y2 - y3) - x3 * y2 + (x3 - x2) * y1))
		+ (((x2 * y3 - x3 * y2) * z1)
		   / (x2 * y3 + x1 * (y2 - y3) - x3 * y2 + (x3 - x2) * y1));

	gp->p1.x = x1;
	gp->p1.y = y1;
	gp->p1.z = z1;
	gp->p2.x = x2;
	gp->p2.y = y2;
	gp->p2.z = z2;
	gp->p3.x = x3;
	gp->p3.y = y3;
	gp->p3.z = z3;

	gp->normv.x = v1->x;
	gp->normv.y = v1->y;
	gp->normv.z = v1->z;

	gp->grad.x = gp->a;
	gp->grad.y = gp->b;
	gp->grad.z = -1;

	vnorm (&gp->normv, &gp->normv);
	vnorm (&gp->grad, &gp->grad);

	gp->normtheta = theta;
	gp->theta = fabs (theta - DTOR (90));
}

void
read_terrain (void)
{
	FILE *inf;
	char buf[1000];
	int idx;
	char *tokstate, *tok;
	double vals[12], v1[3], v2[3], v3[3], theta;
	struct vect v4, v5;

	if ((inf = fopen ("simpleground.raw", "r")) == NULL) {
		fprintf (stderr, "can't open terrain.raw\n");
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

		arrayvsub (v1, vals + 3, vals);
		arrayvsub (v2, vals, vals + 6);
		arrayvcross (v3, v1, v2);

		v4.x = v3[0];
		v4.y = v3[1];
		v4.z = v3[2];

		if (v3[0] == 0 && v3[1] == 0) {
			v5.x = 1;
			v5.y = 1;
		} else {
			v5.x = v3[0];
			v5.y = v3[1];
		}
		v5.z = 0;

		vnorm (&v4, &v4);
		vnorm (&v5, &v5);

		theta = acos (vdot (&v4, &v5));

		if (idx == 9) {
			define_plane (vals[0], vals[1], vals[2],
				      vals[3], vals[4], vals[5],
				      vals[6], vals[7], vals[8],
				      theta, &v4);

			glBindTexture (GL_TEXTURE_2D, texName[0]);
			glBegin (GL_TRIANGLES);
			glTexCoord2f (1, 0);
			glVertex3dv (vals);
			glTexCoord2f (1, 1);
			glVertex3dv (vals + 3);
			glTexCoord2f (0, 1);
			glVertex3dv (vals + 6);

			glNormal3dv (v3);

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
draw_terrain (void)
{
//	glDisable (GL_LIGHTING);

	glCallList (terrain_list);

//	glEnable (GL_LIGHTING);
}


void
draw (void)
{
	double now;
	GLfloat light0_diffuse[] = {1, 1, 1, 0};
	GLfloat light0_position[] = {1, 1, 1, 0};
	GLfloat light1_ambient[] = {.5, .5, .5, 0};
	GLfloat light1_position[] = {0, 0, 1, 0};
	GLfloat light2_diffuse[] = {.1, .1, .1, 0};
	GLfloat light2_position[] = {100, 100, 100, 0};

	now = get_secs ();

	glLoadIdentity ();
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	distanceLookAt (player.p.x, player.p.y, player.p.z + .7, player.camdist,
			playercamera.theta, playercamera.phi);

	color_coords (1);

	glLightfv (GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv (GL_LIGHT0, GL_POSITION, light0_position);
	glLightfv (GL_LIGHT1, GL_AMBIENT, light1_ambient);
	glLightfv (GL_LIGHT1, GL_POSITION, light1_position);
	glLightfv (GL_LIGHT2, GL_AMBIENT, light2_diffuse);
	glLightfv (GL_LIGHT2, GL_POSITION, light2_position);

	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT1);
	glEnable (GL_LIGHT2);

/*	
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_TEXTURE_GEN_S);
	glEnable (GL_TEXTURE_GEN_T);
	glBindTexture (GL_TEXTURE_2D, texName[0]);
	glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 64);	*/

	draw_terrain ();

/*	glDisable (GL_TEXTURE_2D);
	glDisable (GL_TEXTURE_GEN_S);
	glDisable (GL_TEXTURE_GEN_T);*/


	glDisable (GL_LIGHT2);

	if (0) {
		glPushMatrix ();
		glTranslatef (0, 0, 3);
		glEnable (GL_TEXTURE_2D);
		glBindTexture (GL_TEXTURE_2D, texName[0]);
		glBegin (GL_QUADS);
		glTexCoord2f (1, 0);
		glVertex3f (-20, -20, 0);
		glTexCoord2f (1, 1);
		glVertex3f (-20, 20, 0);
		glTexCoord2f (0, 1);
		glVertex3f (20, 20, 0);
		glTexCoord2f (0, 0);
		glVertex3f (20, -20, 0);
		glEnd ();
		glDisable (GL_TEXTURE_2D);
		glPopMatrix ();
	}

	if (0) {
		glPushMatrix ();
		glTranslatef (0, 0, 5);

		glEnable (GL_TEXTURE_2D);
		glEnable (GL_TEXTURE_GEN_S);
		glEnable (GL_TEXTURE_GEN_T);
		glBindTexture (GL_TEXTURE_2D, texName[0]);
		glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 64);

		glutSolidSphere (.5, 20, 20);

		glDisable (GL_TEXTURE_2D);
		glDisable (GL_TEXTURE_GEN_S);
		glDisable (GL_TEXTURE_GEN_T);

		glPopMatrix ();
	}

	glPushMatrix ();
	glTranslatef (player.p.x, player.p.y, player.p.z + .7);

	glRotatef (RTOD (player.theta), 0, 0, 1);
	glRotatef (90, 1, 0, 0);

	glEnable (GL_LIGHT0);
	glutSolidTeapot (1);
	glDisable (GL_LIGHT0);

	glPopMatrix ();

	glFlush ();
	
	SDL_GL_SwapBuffers ();
}

void
leftclickdown (void)
{
	oldmouse_x = mouse_x;
	oldmouse_y = mouse_y;
	SDL_WarpMouse (WIDTH / 2, HEIGHT / 2);
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
}   

void
leftclickup (void)
{
	SDL_WarpMouse (oldmouse_x, oldmouse_y);
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
					misckey[FAKE_Q] = 1;
				break;
			case SDLK_RIGHT:
			case 'd':
				arrowkey[RIGHT] = 1;
				if (mousebutton[3] == 1 || mousebutton[2] == 1)
					misckey[FAKE_E] = 1;
				break;
			case ' ':
				misckey[SPACE] = 1;
				break;
			case 'q':
				misckey[Q] = 1;
				break;
			case 'e':
				misckey[E] = 1;
				break;
			case 'r':
				player.p.x = 0;
				player.p.y = 0;
				break;
			case 'u':
				player.p.z = 200;
				player.mov_vel_z = 0;
				break;
			case 'n':
				player.mov_vel_x = 0;
				player.mov_vel_y = 0;
				player.mov_vel_z = 0;
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
				if (misckey[FAKE_Q])
					misckey[FAKE_Q] = 0;
				break;
			case SDLK_RIGHT:
			case 'd':
				arrowkey[RIGHT] = 0;
				if (misckey[FAKE_E])
					misckey[FAKE_E] = 0;
				break;
			case 'q':
				misckey[Q] = 0;
				break;
			case 'e':
				misckey[E] = 0;
				break;
			case ' ':
				misckey[SPACE] = 0;
			}
		case SDL_MOUSEBUTTONDOWN:
			mousebutton[event.button.button] = 1;
			switch (event.button.button) {
			case 1:
				leftclickdown ();
				if (mousebutton[3]) {
					middleclickdown ();
					mousebutton[2] = 1;
				}
				break;
			case 2:
				middleclickdown ();
				break;
			case 3:
				rightclickdown ();
				if (mousebutton[1]) {
					middleclickdown ();
					mousebutton[2] = 1;
				}
				if (arrowkey[LEFT])
					misckey[FAKE_Q] = 1;
				if (arrowkey[RIGHT])
					misckey[FAKE_E] = 1;
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
				if (mousebutton[2]) {
					mousebutton[2] = 0;
					middleclickup ();
				}
				break;
			case 2:
				middleclickup ();
				break;
			case 3:
				rightclickup ();
				if (mousebutton[2]) {
					mousebutton[2] = 0;
					middleclickup ();
				}
				if (misckey[FAKE_Q])
					misckey[FAKE_Q] = 0;
				if (misckey[FAKE_E])
					misckey[FAKE_E] = 0;
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

int
main (int argc, char **argv)
{
	double now;

	init_sdl_gl_flags (WIDTH, HEIGHT, 0);

	srandom (time (NULL));
	
	init_gl (&argc, argv);

	glEnable (GL_LIGHTING);
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_AUTO_NORMAL);
	glEnable (GL_NORMALIZE);

	glClearDepth (1.0);

	glViewport (0, 0, WIDTH, HEIGHT);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (60.0, (GLfloat) WIDTH/(GLfloat) HEIGHT, .1, 1000.0);
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

	read_terrain ();

	player.p.x = 0;
	player.p.y = 0;
	player.p.z = ground_height (player.p);

	player.mov_vel_x = 0;
	player.mov_vel_y = 0;
	player.mov_vel_z = 0;

	player.mass = 1;
	player.force = 10000;

	player.turnspeed = DTOR (90);
	player.theta = DTOR (97.5);
	player.camdist = 15;

	player.lasttimemov = get_secs ();
	player.lasttimeturn = get_secs ();
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
		if (0) {
			moving ();
		} else {
			new_moving ();
		}

		process_mouse ();

		draw ();

		now = get_secs ();
		
		player.lasttimemov = now;
		player.lasttimeturn = now;

		SDL_Delay (10);
	}

	return (0);
}
