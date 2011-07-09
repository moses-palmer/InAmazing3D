#ifndef CONTEXT_H
#define CONTEXT_H

#include <GL/glew.h>
#include <GL/gl.h>

#include <maze/maze.h>
#include <maze/maze-render.h>

#include <effect.h>
#include <stereo.h>

/**
 * The width of maze walls.
 */
#define MAZE_WALL_WIDTH 0.25

/**
 * The width of the slopes of maze walls.
 */
#define MAZE_SLOPE_WIDTH 0.0

/**
 * The width of the pattern used to render a stereogram.
 */
#define PATTERN_WIDTH 45

/**
 * The height of the pattern used to render a stereogram.
 */
#define PATTERN_HEIGHT 45

/**
 * The z-coordinate of the camera.
 */
#define CAMERA_Z 5.0

/**
 * The z-coordinate of the target.
 */
#define TARGET_Z 0.2

/**
 * The properties of an object in 2D space.
 */
struct context_object {
    /** The position */
    double x, y;
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
 * @param maze_width, maze_height
 *     The dimensions of the maze.
 * @param image_width, image_height
 *     The dimensions of the stereogram image.
 * @param screen_width, screen_height
 *     The dimensions of the screen.
 * @return non-zero upon success and 0 otherwise
 * @see context_free
 */
int
context_initialize(Context *context,
    unsigned int maze_width, unsigned int maze_height,
    unsigned int image_width, unsigned int image_height,
    unsigned int screen_width, unsigned int scren_height);

/**
 * Releases a previously created context.
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

#endif
