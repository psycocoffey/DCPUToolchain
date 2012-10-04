/*****************************************************************************
 * Wave Simulation in OpenGL
 * (C) 2002 Jakob Thomsen
 * http://home.in.tum.de/~thomsen
 * Modified for GLFW by Sylvain Hellegouarch - sh@programmationworld.com
 * Modified for variable frame rate by Marcus Geelnard
 * 2003-Jan-31: Minor cleanups and speedups / MG
 * 2010-10-24: Formatting and cleanup - Camilla Berglund
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define GLFW_INCLUDE_GLU
#include <GL/glfw3.h>

#ifndef M_PI
 #define M_PI 3.1415926535897932384626433832795
#endif

// Maximum delta T to allow for differential calculations
#define MAX_DELTA_T 0.01

// Animation speed (10.0 looks good)
#define ANIMATION_SPEED 10.0

GLfloat alpha = 210.f, beta = -70.f;
GLfloat zoom = 2.f;

GLboolean running = GL_TRUE;
GLboolean locked = GL_FALSE;

int cursorX;
int cursorY;

struct Vertex
{
    GLfloat x, y, z;
    GLfloat r, g, b;
};

#define GRIDW 50
#define GRIDH 50
#define VERTEXNUM (GRIDW*GRIDH)

#define QUADW (GRIDW - 1)
#define QUADH (GRIDH - 1)
#define QUADNUM (QUADW*QUADH)

GLuint quad[4 * QUADNUM];
struct Vertex vertex[VERTEXNUM];

/* The grid will look like this:
 *
 *	3   4	5
 *	*---*---*
 *	|   |	|
 *	| 0 | 1 |
 *	|   |	|
 *	*---*---*
 *	0   1	2
 */

//========================================================================
// Initialize grid geometry
//========================================================================

void init_vertices(void)
{
    int x, y, p;

    // Place the vertices in a grid
    for (y = 0;	 y < GRIDH;  y++)
    {
	for (x = 0;  x < GRIDW;	 x++)
	{
	    p = y * GRIDW + x;

	    vertex[p].x = (GLfloat) (x - GRIDW / 2) / (GLfloat) (GRIDW / 2);
	    vertex[p].y = (GLfloat) (y - GRIDH / 2) / (GLfloat) (GRIDH / 2);
	    vertex[p].z = 0;

	    if ((x % 4 < 2) ^ (y % 4 < 2))
		vertex[p].r = 0.0;
	    else
		vertex[p].r = 1.0;

	    vertex[p].g = (GLfloat) y / (GLfloat) GRIDH;
	    vertex[p].b = 1.f - ((GLfloat) x / (GLfloat) GRIDW + (GLfloat) y / (GLfloat) GRIDH) / 2.f;
	}
    }

    for (y = 0;	 y < QUADH;  y++)
    {
	for (x = 0;  x < QUADW;	 x++)
	{
	    p = 4 * (y * QUADW + x);

	    quad[p + 0] = y	  * GRIDW + x;	   // Some point
	    quad[p + 1] = y	  * GRIDW + x + 1; // Neighbor at the right side
	    quad[p + 2] = (y + 1) * GRIDW + x + 1; // Upper right neighbor
	    quad[p + 3] = (y + 1) * GRIDW + x;	   // Upper neighbor
	}
    }
}

double dt;
double p[GRIDW][GRIDH];
double vx[GRIDW][GRIDH], vy[GRIDW][GRIDH];
double ax[GRIDW][GRIDH], ay[GRIDW][GRIDH];

//========================================================================
// Initialize grid
//========================================================================

void init_grid(void)
{
    int x, y;
    double dx, dy, d;

    for (y = 0; y < GRIDH;  y++)
    {
	for (x = 0; x < GRIDW;	x++)
	{
	    dx = (double) (x - GRIDW / 2);
	    dy = (double) (y - GRIDH / 2);
	    d = sqrt(dx * dx + dy * dy);
	    if (d < 0.1 * (double) (GRIDW / 2))
	    {
		d = d * 10.0;
		p[x][y] = -cos(d * (M_PI / (double)(GRIDW * 4))) * 100.0;
	    }
	    else
		p[x][y] = 0.0;

	    vx[x][y] = 0.0;
	    vy[x][y] = 0.0;
	}
    }
}


//========================================================================
// Draw scene
//========================================================================

void draw_scene(GLFWwindow window)
{
    // Clear the color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // We don't want to modify the projection matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Move back
    glTranslatef(0.0, 0.0, -zoom);
    // Rotate the view
    glRotatef(beta, 1.0, 0.0, 0.0);
    glRotatef(alpha, 0.0, 0.0, 1.0);

    glDrawElements(GL_QUADS, 4 * QUADNUM, GL_UNSIGNED_INT, quad);

    glfwSwapBuffers(window);
}


//========================================================================
// Initialize Miscellaneous OpenGL state
//========================================================================

void init_opengl(void)
{
    // Use Gouraud (smooth) shading
    glShadeModel(GL_SMOOTH);

    // Switch on the z-buffer
    glEnable(GL_DEPTH_TEST);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(struct Vertex), vertex);
    glColorPointer(3, GL_FLOAT, sizeof(struct Vertex), &vertex[0].r); // Pointer to the first color

    glPointSize(2.0);

    // Background color is black
    glClearColor(0, 0, 0, 0);
}


//========================================================================
// Modify the height of each vertex according to the pressure
//========================================================================

void adjust_grid(void)
{
    int pos;
    int x, y;

    for (y = 0; y < GRIDH;  y++)
    {
	for (x = 0;  x < GRIDW;	 x++)
	{
	    pos = y * GRIDW + x;
	    vertex[pos].z = (float) (p[x][y] * (1.0 / 50.0));
	}
    }
}


//========================================================================
// Calculate wave propagation
//========================================================================

void calc_grid(void)
{
    int x, y, x2, y2;
    double time_step = dt * ANIMATION_SPEED;

    // Compute accelerations
    for (x = 0;	 x < GRIDW;  x++)
    {
	x2 = (x + 1) % GRIDW;
	for(y = 0; y < GRIDH; y++)
	    ax[x][y] = p[x][y] - p[x2][y];
    }

    for (y = 0;	 y < GRIDH;  y++)
    {
	y2 = (y + 1) % GRIDH;
	for(x = 0; x < GRIDW; x++)
	    ay[x][y] = p[x][y] - p[x][y2];
    }

    // Compute speeds
    for (x = 0;	 x < GRIDW;  x++)
    {
	for (y = 0;  y < GRIDH;	 y++)
	{
	    vx[x][y] = vx[x][y] + ax[x][y] * time_step;
	    vy[x][y] = vy[x][y] + ay[x][y] * time_step;
	}
    }

    // Compute pressure
    for (x = 1;	 x < GRIDW;  x++)
    {
	x2 = x - 1;
	for (y = 1;  y < GRIDH;	 y++)
	{
	    y2 = y - 1;
	    p[x][y] = p[x][y] + (vx[x2][y] - vx[x][y] + vy[x][y2] - vy[x][y]) * time_step;
	}
    }
}


//========================================================================
// Handle key strokes
//========================================================================

void key_callback(GLFWwindow window, int key, int action)
{
    if (action != GLFW_PRESS)
	return;

    switch (key)
    {
	case GLFW_KEY_ESCAPE:
	    running = 0;
	    break;
	case GLFW_KEY_SPACE:
	    init_grid();
	    break;
	case GLFW_KEY_LEFT:
	    alpha += 5;
	    break;
	case GLFW_KEY_RIGHT:
	    alpha -= 5;
	    break;
	case GLFW_KEY_UP:
	    beta -= 5;
	    break;
	case GLFW_KEY_DOWN:
	    beta += 5;
	    break;
	case GLFW_KEY_PAGE_UP:
	    zoom -= 0.25f;
	    if (zoom < 0.f)
		zoom = 0.f;
	    break;
	case GLFW_KEY_PAGE_DOWN:
	    zoom += 0.25f;
	    break;
	default:
	    break;
    }
}


//========================================================================
// Callback function for mouse button events
//========================================================================

void mouse_button_callback(GLFWwindow window, int button, int action)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT)
	return;

    if (action == GLFW_PRESS)
    {
	glfwSetInputMode(window, GLFW_CURSOR_MODE, GLFW_CURSOR_CAPTURED);
	locked = GL_TRUE;
    }
    else
    {
	locked = GL_FALSE;
	glfwSetInputMode(window, GLFW_CURSOR_MODE, GLFW_CURSOR_NORMAL);
    }
}


//========================================================================
// Callback function for cursor motion events
//========================================================================

void cursor_position_callback(GLFWwindow window, int x, int y)
{
    if (locked)
    {
	alpha += (x - cursorX) / 10.f;
	beta += (y - cursorY) / 10.f;
    }

    cursorX = x;
    cursorY = y;
}


//========================================================================
// Callback function for scroll events
//========================================================================

void scroll_callback(GLFWwindow window, double x, double y)
{
    zoom += (float) y / 4.f;
    if (zoom < 0)
	zoom = 0;
}


//========================================================================
// Callback function for window resize events
//========================================================================

void window_size_callback(GLFWwindow window, int width, int height)
{
    float ratio = 1.f;

    if (height > 0)
	ratio = (float) width / (float) height;

    // Setup viewport
    glViewport(0, 0, width, height);

    // Change to the projection matrix and set our viewing volume
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, ratio, 1.0, 1024.0);
}


//========================================================================
// Callback function for window close events
//========================================================================

static int window_close_callback(GLFWwindow window)
{
    running = GL_FALSE;
    return GL_TRUE;
}


//========================================================================
// main
//========================================================================

int main(int argc, char* argv[])
{
    GLFWwindow window;
    double t, dt_total, t_old;
    int width, height;

    if (!glfwInit())
    {
	fprintf(stderr, "GLFW initialization failed\n");
	exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(key_callback);
    glfwSetWindowCloseCallback(window_close_callback);
    glfwSetWindowSizeCallback(window_size_callback);
    glfwSetMouseButtonCallback(mouse_button_callback);
    glfwSetCursorPosCallback(cursor_position_callback);
    glfwSetScrollCallback(scroll_callback);

    window = glfwCreateWindow(640, 480, GLFW_WINDOWED, "Wave Simulation", NULL);
    if (!window)
    {
	fprintf(stderr, "Could not open window\n");
	exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwGetWindowSize(window, &width, &height);
    window_size_callback(window, width, height);

    glfwSetInputMode(window, GLFW_KEY_REPEAT, GL_TRUE);

    // Initialize OpenGL
    init_opengl();

    // Initialize simulation
    init_vertices();
    init_grid();
    adjust_grid();

    // Initialize timer
    t_old = glfwGetTime() - 0.01;

    while (running)
    {
	t = glfwGetTime();
	dt_total = t - t_old;
	t_old = t;

	// Safety - iterate if dt_total is too large
	while (dt_total > 0.f)
	{
	    // Select iteration time step
	    dt = dt_total > MAX_DELTA_T ? MAX_DELTA_T : dt_total;
	    dt_total -= dt;

	    // Calculate wave propagation
	    calc_grid();
	}

	// Compute height of each vertex
	adjust_grid();

	// Draw wave grid to OpenGL display
	draw_scene(window);

	glfwPollEvents();
    }

    exit(EXIT_SUCCESS);
}
