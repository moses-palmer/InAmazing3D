#include <math.h>
#include <stdlib.h>

#include "context.h"

/* The strength values for the different effects */
#define WAVE_STRENGTH_BASE 5.0
#define WAVE_STRENGTH_EXTRA 8.0
#define LUMINANCE_STRENGTH1_BASE 2.0
#define LUMINANCE_STRENGTH1_EXTRA 4.0
#define LUMINANCE_STRENGTH2_BASE 2.0
#define LUMINANCE_STRENGTH2_EXTRA 4.0

/**
 * @see gluPerspective
 */
static void
mgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan(fovy * M_PI / 360.0);
   ymin = -ymax;
   xmin = ymin * aspect;
   xmax = ymax * aspect;


   glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

/**
 * @see gluLookAt
 */
static void
mgluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
    GLfloat centerx, GLfloat centery, GLfloat centerz,
    GLfloat upx, GLfloat upy, GLfloat upz)
{
    GLfloat m[16];
    GLfloat x[3], y[3], z[3];
    GLfloat mag;

    z[0] = eyex - centerx;
    z[1] = eyey - centery;
    z[2] = eyez - centerz;
    mag = sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
    if (mag) {
        z[0] /= mag;
        z[1] /= mag;
        z[2] /= mag;
    }

    y[0] = upx;
    y[1] = upy;
    y[2] = upz;

    x[0] = y[1] * z[2] - y[2] * z[1];
    x[1] = -y[0] * z[2] + y[2] * z[0];
    x[2] = y[0] * z[1] - y[1] * z[0];

    y[0] = z[1] * x[2] - z[2] * x[1];
    y[1] = -z[0] * x[2] + z[2] * x[0];
    y[2] = z[0] * x[1] - z[1] * x[0];

    mag = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    if (mag) {
        x[0] /= mag;
        x[1] /= mag;
        x[2] /= mag;
    }

    mag = sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    if (mag) {
        y[0] /= mag;
        y[1] /= mag;
        y[2] /= mag;
    }

#define M(row,col)  m[col*4+row]
    M(0, 0) = x[0];
    M(0, 1) = x[1];
    M(0, 2) = x[2];
    M(0, 3) = 0.0;
    M(1, 0) = y[0];
    M(1, 1) = y[1];
    M(1, 2) = y[2];
    M(1, 3) = 0.0;
    M(2, 0) = z[0];
    M(2, 1) = z[1];
    M(2, 2) = z[2];
    M(2, 3) = 0.0;
    M(3, 0) = 0.0;
    M(3, 1) = 0.0;
    M(3, 2) = 0.0;
    M(3, 3) = 1.0;
#undef M
    glMultMatrixf(m);

    /* Translate Eye to Origin */
    glTranslatef(-eyex, -eyey, -eyez);
}

int
context_initialize(Context *context,
    unsigned int maze_width, unsigned int maze_height,
    unsigned int image_width, unsigned int image_height,
    unsigned int screen_width, unsigned int screen_height)
{
    double luminance_strengths1[5];
    double luminance_strengths2[5];
    double wave_strengths[2 * 4];
    int i;
    StereoPattern *pattern_base, *pattern;

    /* Make sure that the context is passed */
    if (!context) {
        return 0;
    }

    /* Initialise the maze */
    context->maze.data = maze_create(maze_width, maze_height);
    if (!context->maze.data) {
        return 0;
    }
    maze_initialize_randomized_prim(context->maze.data, NULL, NULL);

    /* Initialise the stereogram z-buffer */
    context->stereo.zbuffer = stereo_zbuffer_create(image_width, image_height,
        1);

    /* Randomise the effect parameters */
    for (i = 0; i < sizeof(wave_strengths) / sizeof(double); i++) {
        wave_strengths[i] = WAVE_STRENGTH_BASE + WAVE_STRENGTH_EXTRA
            * (double)(rand() - RAND_MAX / 2) / RAND_MAX / (i + 1);
    }
    for (i = 0; i < sizeof(luminance_strengths1) / sizeof(double); i++) {
        luminance_strengths1[i] = LUMINANCE_STRENGTH1_BASE
                + LUMINANCE_STRENGTH1_EXTRA
            * (double)(rand() - RAND_MAX / 2) / RAND_MAX / (i + 1);
    }
    for (i = 0; i < sizeof(luminance_strengths2) / sizeof(double); i++) {
        luminance_strengths2[i] = LUMINANCE_STRENGTH2_BASE
                + LUMINANCE_STRENGTH2_EXTRA
            * (double)(rand() - RAND_MAX / 2) / RAND_MAX / (i + 1);
    }

    /* Create the base image for the pattern */
    pattern_base = stereo_pattern_create(PATTERN_WIDTH, PATTERN_HEIGHT);
    stereo_pattern_effect_run(pattern_base, luminance,
        sizeof(luminance_strengths1) / sizeof(double), luminance_strengths1,
        PP_RED | PP_BLUE);
    stereo_pattern_effect_run(pattern_base, luminance,
        sizeof(luminance_strengths1) / sizeof(double), luminance_strengths1,
        PP_RED);
    stereo_pattern_effect_run(pattern_base, luminance,
        sizeof(luminance_strengths2) / sizeof(double), luminance_strengths2,
        PP_GREEN);

    /* Initialise the effect */
    pattern = stereo_pattern_create(
        PATTERN_WIDTH, PATTERN_HEIGHT);
    context->stereo.effect = stereo_pattern_effect_wave(pattern,
        sizeof(wave_strengths) / sizeof(double) / 2, wave_strengths,
        pattern_base);
    stereo_pattern_effect_apply(context->stereo.effect);

    /* Initialise the stereogram image */
    context->stereo.image = stereo_image_create_from_zbuffer(
        context->stereo.zbuffer, pattern, 10.0, 1);

    /* Initialise the OpenGL data */
    context->gl.ratio = (GLfloat)screen_width / screen_height;

    /* Initialise the camera and target */
    context->camera.x = context->target.x = maze_width / 2.0;
    context->camera.y = context->target.y = maze_height / 2.0;

    return 1;
}

/**
 * Sets up the camera for the context.
 *
 * The current OpenGL matrix is modified.
 *
 * @param context
 *     The context.
 */
static void
camera_setup(Context *context)
{
    glMatrixMode(GL_MODELVIEW);
    mgluPerspective(45, context->gl.ratio, CAMERA_Z - 1.5, CAMERA_Z + 0.5);
    mgluLookAt(
        context->camera.x,
        context->camera.y,
        CAMERA_Z,

        context->target.x,
        context->target.y,
        TARGET_Z,

        0.0, 1.0, 0.0);
}

/**
 * Sets up the lights for the context.
 *
 * @param context
 *     The context.
 */
static void
lights_setup(Context *context)
{
    GLfloat light_position[] = {
        context->camera.x + 1.0,
        context->camera.y + 1.0,
        (CAMERA_Z - TARGET_Z) / 2.0};
    static GLfloat light_ambient[] = {0.0, 0.0, 0.0, 1.0};
    static GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
    static GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};
    static GLfloat material_specular[] = {0.0, 0.0, 0.0, 1.0};
    static GLfloat material_emission[] = {0.0, 0.0, 0.0, 1.0};

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_emission);
}

void
context_free(Context *context)
{
    /* Make sure that the context is passed */
    if (!context) {
        return;
    }

    if (context->maze.data) {
        maze_free(context->maze.data);
        context->maze.data = NULL;
    }

    if (context->stereo.zbuffer) {
        stereo_zbuffer_free(context->stereo.zbuffer);
        context->stereo.zbuffer = NULL;
    }

    if (context->stereo.effect) {
        stereo_pattern_effect_free(context->stereo.effect);
        context->stereo.effect = NULL;
    }

    if (context->stereo.image) {
        stereo_image_free(context->stereo.image);
        context->stereo.image = NULL;
    }
}

void
context_render(Context *context)
{
    camera_setup(context);
    lights_setup(context);
}
