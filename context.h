#ifndef CONTEXT_H
#define CONTEXT_H

#include <GL/gl.h>

#include <maze/maze.h>
#include <maze/maze-render.h>

#include <effect.h>
#include <stereo.h>

/**
 * The width of the pattern used to render a stereogram.
 */
#define PATTERN_WIDTH 45

/**
 * The height of the pattern used to render a stereogram.
 */
#define PATTERN_HEIGHT 45

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
 * @return non-zero upon success and 0 otherwise
 * @see context_free
 */
int
context_initialize(Context *context,
    unsigned int maze_width, unsigned int maze_height,
    unsigned int image_width, unsigned int image_height);

/**
 * Releases a previously created context.
 */
void
context_free(Context *context);

#endif
