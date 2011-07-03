#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
    #include <SDL/SDL.h>
#else
    #include <SDL.h>
#endif

#include "context.h"

/**
 * The width of the maze.
 */
#define MAZE_WIDTH 30

/**
 * The height of the maze.
 */
#define MAZE_HEIGHT 20

/**
 * The width of the stereogram image.
 */
#define IMAGE_WIDTH 512

/**
 * The height of the stereogram image.
 */
#define IMAGE_HEIGHT 512

/**
 * Handles any pending SDL events.
 *
 * @return non-zero if the application should continue running and 0 otherwise
 */
static int
handle_events(Context *context)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        /* Exit if the window is closed */
        case SDL_QUIT:
            return 0;

        /* Check for keypresses */
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                return 0;

            /* Prevent compiler warning */
            default: break;
            }
            break;

        /* Prevent compiler warning */
        default: break;
        }
    }

    return 1;
}

/**
 * Updates the display.
 */
static void
do_display(Context *context)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    /* Render the context */
    context_render(context);

    /* Render to screen */
    SDL_GL_SwapBuffers();
}

/**
 * Initialises OpenGL for the specified resolution.
 *
 * @param width, height
 *     The width and height of the viewport.
 */
static void
opengl_initialize(int width, int height)
{
    /* Culling. */
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);

    /* Enable depth test */
    glEnable(GL_DEPTH_TEST);

    /* Set the clear color. */
    glClearColor(0, 0, 0, 0);

    /* Setup our viewport. */
    glViewport(0, 0, width, height);
}

int
main(int argc, char *argv[])
{
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("Unable to init SDL: %s\n", SDL_GetError());
        return 1;
    }
    atexit(SDL_Quit);

    /* Get video information */
    const SDL_VideoInfo *vinfo = SDL_GetVideoInfo();
    if (!vinfo) {
        printf("Unable to get video info: %s\n", SDL_GetError());
        return 1;
    }

    /* Initialise the screen */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Surface* screen = SDL_SetVideoMode(vinfo->current_w, vinfo->current_h,
        32, SDL_OPENGL | SDL_FULLSCREEN);
    if (!screen) {
        printf("Unable to set %dx%d video: %s\n",
            vinfo->current_w, vinfo->current_h, SDL_GetError());
        return 1;
    }

    /* Setup OpenGL */
    opengl_initialize(vinfo->current_w, vinfo->current_h);

    /* Initialise the context */
    Context context;
    memset(&context, 0, sizeof(context));
    if (!context_initialize(&context, MAZE_WIDTH, MAZE_HEIGHT,
            IMAGE_WIDTH, IMAGE_HEIGHT)) {
        printf("Unable to initialise context.\n");
        return 1;
    }

    /* Enter the main loop */
    while (handle_events(&context)) {
        do_display(&context);
    }

    context_free(&context);

    return 0;
}
