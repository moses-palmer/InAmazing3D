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

/** The precision of the sphere approximation */
#define SPHERE_PRECISION 20

/** The margin of the target */
#define TARGET_MARGIN (MAZE_WALL_WIDTH + MAZE_SLOPE_WIDTH)
#define ITARGET_MARGIN (1.0 - TARGET_MARGIN)

/**
 * Updates the speed and position according to the current values.
 *
 * @param object
 *     The context object to move.
 * @param resistance
 *     The resistance to movement. The objet speed is multiplied with this
 *     value.
 */
static void
context_object_move(struct context_object *object, double resistance)
{
    object->x = object->x + object->vx;
    object->y = object->y + object->vy;

    object->vx = resistance * (object->vx + object->ax);
    object->vy = resistance * (object->vy + object->ay);
}

/**
 * Makes an object target a position.
 *
 * Its acceleration is set to the difference between the current position and
 * the target multiplied by a.
 *
 * @param object
 *     The context object to target.
 * @param x, y
 *     The position to target.
 * @param a
 *     The strength of the acceleration towards (x, y). This should be a value
 *     less than 1.0.
 */
static void
context_object_target(struct context_object *object,
    double x, double y, double a)
{
    object->ax = a * (x - object->x);
    object->ay = a * (y - object->y);
}

/**
 * Render a context object on screen.
 *
 * The object is rendered as a sphere approximation.
 *
 * @param object
 *     The object to render.
 */
static void
context_object_render(const Context *context)
{
    int i, j;

    glPushMatrix();

    glTranslatef(context->target.x,
        context->maze.data->height - context->target.y, 0.7);
    glScalef(0.2, 0.2, 0.2);

    for (i = 0; i < SPHERE_PRECISION / 2; i++) {
        GLfloat theta1, theta2;
        theta1 = i * 2.0 * M_PI / SPHERE_PRECISION - M_PI / 2.0;
        theta2 = (i + 1) * 2.0 * M_PI / SPHERE_PRECISION - M_PI / 2.0;

        glBegin(GL_TRIANGLE_STRIP);
        for (j = 0; j <= SPHERE_PRECISION; j++) {
            GLfloat theta3 = j * 2.0 * M_PI / SPHERE_PRECISION;
            GLfloat x, y, z;

            x = cos(theta1) * cos(theta3);
            y = sin(theta1);
            z = cos(theta1) * sin(theta3);
            glNormal3f(x, y, z);
            glVertex3f(x, y, z);

            x = cos(theta2) * cos(theta3);
            y = sin(theta2);
            z = cos(theta2) * sin(theta3);
            glNormal3f(x, y, z);
            glVertex3f(x, y, z);
        }
        glEnd();
    }

    glPopMatrix();
}

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
    maze_door_open(context->maze.data,
        0, 0,
        MAZE_WALL_LEFT);
    maze_door_open(context->maze.data,
        context->maze.data->width - 1, context->maze.data->height - 1,
        MAZE_WALL_RIGHT);

    for (i = 0; i < 4 * maze_width * maze_height * MAZE_SHORTCUT_RATIO; i++) {
        int x = rand() % maze_width;
        int y = rand() % maze_height;
        int wall = rand() % 4;

        switch (wall) {
        case 0:
            if (x > 0) {
                maze_door_open(context->maze.data, x, y, MAZE_WALL_LEFT);
            }
            break;
        case 1:
            if (x < maze_width - 1) {
                maze_door_open(context->maze.data, x, y, MAZE_WALL_RIGHT);
            }
            break;
        case 2:
            if (y > 0) {
                maze_door_open(context->maze.data, x, y, MAZE_WALL_UP);
            }
            break;
        case 3:
            if (y < maze_height - 1) {
                maze_door_open(context->maze.data, x, y, MAZE_WALL_DOWN);
            }
            break;
        }
    }

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

    /* Automatically update the pattern every frame */
    context->stereo.update_pattern = 1;

    /* Initialise the OpenGL data */
    context->gl.ratio = (GLfloat)screen_width / screen_height;
    glGenFramebuffers(sizeof(context->gl.framebuffers) / sizeof(GLuint),
        context->gl.framebuffers);
    glGenRenderbuffers(sizeof(context->gl.renderbuffers) / sizeof(GLuint),
        context->gl.renderbuffers);
    glGenTextures(sizeof(context->gl.textures) / sizeof(GLuint),
        context->gl.textures);
    context->gl.render_stereo = 1;

    /* Specify the renderbuffer */
    GLuint renderbuffer = context->gl.renderbuffers[0];
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
        image_width, image_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    /* Specify the render buffer */
    GLuint framebuffer = context->gl.framebuffers[0];
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER_EXT, renderbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* Initialise the camera and target */
    context->camera.x = context->target.x = 0.0;
    context->camera.y = context->target.y = 0.5;
    context->camera.vx = context->target.vx = 0.0;
    context->camera.vy = context->target.vy = 0.0;
    context->camera.ax = context->target.ax = 0.0;
    context->camera.ay = context->target.ay = 0.0;

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
    mgluPerspective(45, context->gl.ratio, CAMERA_Z - 1.5, CAMERA_Z + 1.0);
    mgluLookAt(
        context->camera.x,
        context->maze.data->height - context->camera.y,
        CAMERA_Z,

        context->target.x,
        context->maze.data->height - context->target.y,
        TARGET_Z,

        0.1, 1.0, 0.0);
}

/**
 * Sets up the lights for the context.
 *
 * @param context
 *     The context.
 */
static void
lights_setup(Context *context, int enable)
{
    if (enable) {
        GLfloat light_position[] = {
            2.0 * context->camera.ax,
            -2.0 * context->camera.ay,
            -1.0, 1.0};
        static GLfloat light_ambient[] = {0.0, 0.0, 0.0, 1.0};
        static GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
        static GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};
        static GLfloat material_emission[] = {0.0, 0.0, 0.0, 1.0};
        static GLfloat material_diffuse[] = {0.0, 0.0, 0.0, 1.0};
        static GLfloat material_specular[] = {0.0, 0.0, 0.0, 1.0};

        /* Select the correct matrix for "light at eye" mode */
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);

        glEnable(GL_COLOR_MATERIAL);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_emission);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_specular);

        glPopMatrix();
    }
    else {
        glDisable(GL_LIGHTING);
        glDisable(GL_LIGHT0);

        glDisable(GL_COLOR_MATERIAL);
    }
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

    glDeleteTextures(sizeof(context->gl.textures) / sizeof(GLuint),
        context->gl.textures);
    glDeleteRenderbuffers(sizeof(context->gl.renderbuffers) / sizeof(GLuint),
        context->gl.renderbuffers);
    glDeleteFramebuffers(sizeof(context->gl.framebuffers) / sizeof(GLuint),
        context->gl.framebuffers);
}

/**
 * Renders the scene in stereogram mode.
 *
 * This will make the maze be displayed as an animated stereogram.
 *
 * @param context
 *     The context.
 */
static void
context_render_stereo(Context *context)
{
    /* Determine the size of the texture */
    GLsizei width, height;
    width = context->stereo.zbuffer->width;
    height = context->stereo.zbuffer->height;

    /* Bind the frame buffer and the render buffer */
    GLuint framebuffer = context->gl.framebuffers[0];
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    GLuint renderbuffer = context->gl.renderbuffers[0];
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);

    /* Clear the buffer */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Store the old viewport and set one the size of the texture */
    GLint old_viewport[4];
    glGetIntegerv(GL_VIEWPORT, old_viewport);
    glViewport(0, 0, width, height);

    /* Draw the maze with a floor */
    maze_render_gl(context->maze.data, MAZE_WALL_WIDTH, MAZE_SLOPE_WIDTH,
        1, (int)context->camera.x, (int)context->camera.y, 5);
    context_object_render(context);

    /* Retrieve the depth data to the z-buffer */
    glPixelStorei(GL_PACK_ROW_LENGTH, context->stereo.zbuffer->rowoffset);
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT,
        GL_UNSIGNED_BYTE, context->stereo.zbuffer->data);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* Update the pattern if required and regenerate the stereogram from the
       depth data generated by OpenGL */
    if (context->stereo.update_pattern) {
        stereo_pattern_effect_apply(context->stereo.effect);
    }
    stereo_image_apply(context->stereo.image, context->stereo.zbuffer, 0);

    /* Clear the depth buffer to enable the texture to be displayed */
    glClear(GL_DEPTH_BUFFER_BIT);

    /* Activate the stereogram texture */
    glEnable(GL_TEXTURE_2D);
    GLuint stereogram_texture = context->gl.textures[0];
    glBindTexture(GL_TEXTURE_2D, stereogram_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, context->stereo.image->image->pixels);

    /* Restore the matrix and the viewport */
    glLoadIdentity();
    glViewport(old_viewport[0], old_viewport[1],
        old_viewport[2], old_viewport[3]);

    /* Draw a rectangle with the stereogram as texture */
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(-1.0, -1.0);
    glTexCoord2f(1.0, 0.0);
    glVertex2f(1.0, -1.0);
    glTexCoord2f(1.0, 1.0);
    glVertex2f(1.0, 1.0);
    glTexCoord2f(0.0, 1.0);
    glVertex2f(-1.0, 1.0);
    glEnd();
}

/**
 * Renders the scene in plain 3D mode.
 *
 * @param context
 *     The context.
 */
static void
context_render_plain(Context *context)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_TEXTURE_2D);
    maze_render_gl(context->maze.data, MAZE_WALL_WIDTH, MAZE_SLOPE_WIDTH,
        1, (int)context->camera.x, (int)context->camera.y, 5);
    context_object_render(context);
}

void
context_render(Context *context)
{
    camera_setup(context);
    lights_setup(context, !context->gl.render_stereo);

    if (context->gl.render_stereo) {
        context_render_stereo(context);
    }
    else {
        context_render_plain(context);
    }
}

void
context_camera_move(Context *context)
{
    context_object_target(&context->camera,
        context->target.x, context->target.y, 0.7);
    context_object_move(&context->camera, 0.1);
}

void
context_target_accelerate(Context *context, double ax, double ay)
{
    context->target.ax = ax;
    context->target.ay = ay;
}

void
context_target_move(Context *context)
{
    /* Retrieve the old room location and the position within the room */
    int ox, oy;
    double ofx, ofy;
    ox = (int)context->target.x;
    oy = (int)context->target.y;
    ofx = context->target.x - ox;
    ofy = context->target.y - oy;

    /* Calculate the new coordinates */
    context_object_move(&context->target, 0.2);
    if (context->target.x < -0.5) {
        context->target.x = -0.5;
    }
    else if (context->target.x >= context->maze.data->width - 0.01) {
        /* Make sure (int)context->target.x < context->maze.data->width */
        context->target.x = context->maze.data->width - 0.01;
    }

    /* Retrieve the new room location and the position within the room */
    int x, y;
    double fx, fy;
    x = (int)context->target.x;
    y = (int)context->target.y;
    fx = context->target.x - x;
    fy = context->target.y - y;

    /* Determine what edges we have moved into */
    int edges = 0
        | (fx < TARGET_MARGIN ? MAZE_WALL_LEFT : 0)
        | (fx > ITARGET_MARGIN ? MAZE_WALL_RIGHT : 0)
        | (fy < TARGET_MARGIN ? MAZE_WALL_UP : 0)
        | (fy > ITARGET_MARGIN ? MAZE_WALL_DOWN : 0);

    /* Handle bumping into walls of this room */
    if (edges & MAZE_WALL_LEFT
            && !maze_is_open_left(context->maze.data, x, y)) {
        context->target.x = x + TARGET_MARGIN;
        fx = TARGET_MARGIN;
        edges &= ~MAZE_WALL_LEFT;
    }
    else if (edges & MAZE_WALL_RIGHT
            && !maze_is_open_right(context->maze.data, x, y)) {
        context->target.x = x + ITARGET_MARGIN;
        fx = ITARGET_MARGIN;
        edges &= ~MAZE_WALL_RIGHT;
    }
    if (edges & MAZE_WALL_UP
            && !maze_is_open_up(context->maze.data, x, y)) {
        context->target.y = y + TARGET_MARGIN;
        fy = TARGET_MARGIN;
        edges &= ~MAZE_WALL_UP;
    }
    else if (edges & MAZE_WALL_DOWN
            && !maze_is_open_down(context->maze.data, x, y)) {
        context->target.y = y + ITARGET_MARGIN;
        fy = ITARGET_MARGIN;
        edges &= ~MAZE_WALL_DOWN;
    }

    /* Handle bumping into corners */
    if ((edges & MAZE_CORNER_UP_LEFT) == MAZE_CORNER_UP_LEFT
            && maze_is_corner_up_left_out(context->maze.data, x, y)) {
        if (fx > fy) {
            context->target.x = x + TARGET_MARGIN;
            edges &= ~MAZE_WALL_LEFT;
        }
        else {
            context->target.y = y + TARGET_MARGIN;
            edges &= ~MAZE_WALL_UP;
        }
    }
    else if ((edges & MAZE_CORNER_UP_RIGHT) == MAZE_CORNER_UP_RIGHT
            && maze_is_corner_up_right_out(context->maze.data, x, y)) {
        if (1.0 - fx > fy) {
            context->target.x = x + ITARGET_MARGIN;
            edges &= ~MAZE_WALL_RIGHT;
        }
        else {
            context->target.y = y + TARGET_MARGIN;
            edges &= ~MAZE_WALL_UP;
        }
    }
    else if ((edges & MAZE_CORNER_DOWN_LEFT) == MAZE_CORNER_DOWN_LEFT
            && maze_is_corner_down_left_out(context->maze.data, x, y)) {
        if (1.0 - fx < fy) {
            context->target.x = x + TARGET_MARGIN;
            edges &= ~MAZE_WALL_LEFT;
        }
        else {
            context->target.y = y + ITARGET_MARGIN;
            edges &= ~MAZE_WALL_DOWN;
        }
    }
    else if ((edges & MAZE_CORNER_DOWN_RIGHT) == MAZE_CORNER_DOWN_RIGHT
            && maze_is_corner_down_right_out(context->maze.data, x, y)) {
        if (fx < fy) {
            context->target.x = x + ITARGET_MARGIN;
            edges &= ~MAZE_WALL_RIGHT;
        }
        else {
            context->target.y = y + ITARGET_MARGIN;
            edges &= ~MAZE_WALL_DOWN;
        }
    }
}
