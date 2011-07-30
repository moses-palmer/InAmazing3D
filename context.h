#ifndef CONTEXT_H
#define CONTEXT_H

#include <GL/glew.h>
#include <GL/gl.h>

#include <maze/maze.h>
#include <maze/maze-render.h>

#include <effect.h>
#include <stereo.h>

/**
 * The z-coordinate of the camera.
 */
#define CAMERA_Z 3.5

/**
 * The z-coordinate of the target.
 */
#define TARGET_Z 0.7

/**
 * The properties of an object in 2D space.
 */
struct context_object {
    /** The position */
    double x, y;

    /** The velocity */
    double vx, vy;

    /** The acceleration */
    double ax, ay;
};

typedef struct {
    /**
     * The maze that we are rendering.
     */
    struct {
        /** The maze data */
        Maze *data;
    } maze;

    /**
     * The stereogram image.
     */
    struct {
        /** The z-buffer of the stereogram image */
        ZBuffer *zbuffer;

        /** The pattern effect to apply to the pattern continuously */
        StereoPatternEffect *effect;

        /** The stereogram image */
        StereoImage *image;

        /** Whether to update the pattern for every frame */
        int update_pattern;
    } stereo;

    /**
     * Data used by OpenGL.
     */
     struct {
         /** The ratio screen_width / screen_height */
        GLfloat ratio;

        /** The frame buffers used */
        GLuint framebuffers[1];

        /** The render buffers used */
        GLuint renderbuffers[1];

        /** The thextures used */
        GLuint textures[1];

        /** Whether to render a stereogram */
        int render_stereo;
    } gl;

    /**
     * The location of the camera.
     *
     * The camera floats above the ground.
     */
    struct context_object camera;

    /**
     * The location of the target.
     *
     * The target is located on the ground.
     */
    struct context_object target;
} Context;

/**
 * Initialises a context.
 *
 * The maze, stereogram and z-buffer fields are created.
 *
 * If this function completes sucessfully, context_free must be called.
 *
 * @param context
 *     The context to initialise.
 * @param image_width, image_height
 *     The dimensions of the stereogram image.
 * @param screen_width, screen_height
 *     The dimensions of the screen.
 * @param pattern_base
 *     The background pattern for the stereogram. If this function returns
 *     non-zero, ownership of this pattern is assumed by the context, and it
 *     should not be freed.
 * @return non-zero upon success and 0 otherwise
 * @see context_free
 */
int
context_initialize(Context *context,
    unsigned int image_width, unsigned int image_height,
    unsigned int screen_width, unsigned int screen_height,
    StereoPattern *pattern_base);

/**
 * Releases a previously created context.
 *
 * @param context
 *     The context.
 */
void
context_free(Context *context);

/**
 * Renders the context on screen.
 *
 * @param context
 *     The context.
 */
void
context_render(Context *context);

/**
 * Moves the camera towards the target.
 *
 * @param context
 *     The context.
 */
void
context_camera_move(Context *context);

/**
 * Updates the horizontal acceleration of the context target.
 *
 * @param context
 *     The context whose target to manipulate.
 * @param a
 *     The acceleration.
 */
void
context_target_accelerate_x(Context *context, double a);

/**
 * Updates the vertical acceleration of the context target.
 *
 * @param context
 *     The context whose target to manipulate.
 * @param a
 *     The acceleration.
 */
void
context_target_accelerate_y(Context *context, double a);

/**
 * Moves the target.
 *
 * The velocity and direction are taken from the target struct.
 *
 * @param context
 *     The context.
 */
void
context_target_move(Context *context);

#endif
