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
#define MAZE_WALL_WIDTH 0.05

/**
 * The width of the slopes of maze walls.
 */
#define MAZE_SLOPE_WIDTH 0.25

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
#define CAMERA_Z 3.5

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

    /** The velocity */
    double vx, vy;

    /** The acceleration */
    double ax, ay;
};

/**
 * Updates the speed and position according to the current values.
 *
 * @param object
 *     The context object to move.
 * @param resistance
 *     The resistance to movement. The objet speed is multiplied with this
 *     value.
 */
void
context_object_move(struct context_object *object, double resistance);

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
void
context_object_target(struct context_object *object,
    double x, double y, double a);

/**
 * Changes the acceleration of an object.
 *
 * Its acceleration is multiplied by a.
 *
 * @param object
 *     The context object to target.
 * @param a
 *     The strength of the accleration. A value less than 1.0 will decrease the
 *     acceleration, and a value greater than 1.0 will increase it.
 */
void
context_object_accelerate(struct context_object *object, GLfloat a);

/**
 * Render a context object on screen.
 *
 * The object is rendered as a sphere approximation.
 *
 * @param object
 *     The object to render.
 */
void
context_object_render(const struct context_object *object);

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

/**
 * Moves the camera towards the target.
 *
 * @param context
 *     The context.
 */
void
context_camera_move(Context *context);

/**
 * Updates the acceleration of the context target.
 *
 * @param context
 *     The context whose target to manipulate.
 * @param ax, ay
 *     The horizontal and vertical acceleration.
 */
void
context_target_accelerate(Context *context, double ax, double ay);

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
