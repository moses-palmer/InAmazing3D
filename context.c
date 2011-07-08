#include <stdlib.h>

#include "context.h"

/* The strength values for the different effects */
#define WAVE_STRENGTH_BASE 5.0
#define WAVE_STRENGTH_EXTRA 8.0
#define LUMINANCE_STRENGTH1_BASE 2.0
#define LUMINANCE_STRENGTH1_EXTRA 4.0
#define LUMINANCE_STRENGTH2_BASE 2.0
#define LUMINANCE_STRENGTH2_EXTRA 4.0

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

    return 1;
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
}
