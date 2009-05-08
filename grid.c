#include "alex.h"
#include "alexgl.h"

static gboolean
deleteEvent (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	g_print("Recieved delete event\n");

	return FALSE;
}

double
get_secs (void)
{
	struct timeval tv;
	static double start;
	double now;

	gettimeofday (&tv, NULL);
	now = tv.tv_sec + tv.tv_usec / 1e6;
	if (start == 0)
		start = now;
	return (now - start);
}

static gboolean
exposeCb (GtkWidget *drawingArea, GdkEventExpose *event, gpointer userData)
{
	GdkGLContext *glContext = gtk_widget_get_gl_context (drawingArea);
	GdkGLDrawable *glDrawable = gtk_widget_get_gl_drawable (drawingArea);
	double t, x;

	if (!gdk_gl_drawable_gl_begin (glDrawable, glContext))
		g_assert_not_reached ();

	glClearColor (0.0, 0.0, 0.0, 0.0);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glColor3f (1.0, 1.0, 1.0);

	const gboolean SOLID = TRUE;
	const gdouble SCALE = 0.5;

	static GLfloat light0_position[] = {1.0,1.0,1.0,0.0};
	glLightfv (GL_LIGHT0, GL_POSITION, light0_position);
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT0);

	t = get_secs ();

	glPushMatrix ();

	x = 45 * sin (t * 1 * M_PI);

	if (x >= 0)
		x = -x;
	glRotatef (x, 0, 0, 1);

	gdk_gl_draw_teapot (SOLID, SCALE);

	glPopMatrix ();

	if (gdk_gl_drawable_is_double_buffered (glDrawable))
		gdk_gl_drawable_swap_buffers (glDrawable);
	else
		glFlush ();

	gdk_gl_drawable_gl_end (glDrawable);

	return TRUE;
}

static gboolean
idleCb (gpointer userData)
{
	GtkWidget *drawingArea = GTK_WIDGET (userData);

	gdk_window_invalidate_rect (drawingArea->window,
				    &drawingArea->allocation, FALSE);

	return TRUE;
}

static gboolean
configureCb (GtkWidget *drawingArea, GdkEventConfigure *event,
	     gpointer user_data)
{
	GdkGLContext *glContext = gtk_widget_get_gl_context (drawingArea);
	GdkGLDrawable *glDrawable = gtk_widget_get_gl_drawable (drawingArea);

	if (!gdk_gl_drawable_gl_begin (glDrawable, glContext))
		g_assert_not_reached ();

	glViewport (0, 0, drawingArea->allocation.width,
		    drawingArea->allocation.height);

	gdk_gl_drawable_gl_end (glDrawable);

	return TRUE;
}

static void initGl (int argc, char **argv, GtkWidget *drawingArea)
{
	GdkGLConfig *glConfig;
	gtk_gl_init (&argc, &argv);

	glConfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGBA |
					      GDK_GL_MODE_DEPTH |
					      GDK_GL_MODE_DOUBLE);
	if (!glConfig)
		g_assert_not_reached ();

	if (!gtk_widget_set_gl_capability (drawingArea, glConfig, NULL, TRUE,
					   GDK_GL_RGBA_TYPE))
		g_assert_not_reached ();

	g_signal_connect (drawingArea, "configure-event", G_CALLBACK
			  (configureCb), NULL);
	g_signal_connect (drawingArea, "expose-event", G_CALLBACK (exposeCb),
			  NULL);

	const gdouble TIMEOUT_PERIOD = 1000 / 50.0;

	g_timeout_add (TIMEOUT_PERIOD, idleCb, drawingArea);
}

void initWindow (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *drawingArea;

	gtk_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (window), 400, 400);
	drawingArea = gtk_drawing_area_new ();

	gtk_container_add (GTK_CONTAINER (window), drawingArea);

	g_signal_connect_swapped (window, "destroy",
				  G_CALLBACK (gtk_main_quit), NULL);
	g_signal_connect (G_OBJECT (window), "delete-event",
			  G_CALLBACK (deleteEvent), NULL);

	gtk_widget_set_events (drawingArea, GDK_EXPOSURE_MASK);

	initGl (argc, argv, drawingArea);
	gtk_widget_show_all (window);
}

int
main (int argc, char **argv)
{
	initWindow (argc, argv);

	gtk_main ();

	return (0);
}
