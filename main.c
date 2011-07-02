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

static void
do_display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    /* Render to screen */
    SDL_GL_SwapBuffers();
}

int
main(int argc, char *argv[])
{
    Context context;
    const SDL_VideoInfo *vinfo;
    int done;

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("Unable to init SDL: %s\n", SDL_GetError());
        return 1;
    }

    vinfo = SDL_GetVideoInfo();
    if (!vinfo) {
        printf("Unable to get video info: %s\n", SDL_GetError());
        return 1;
    }

    /* Make sure SDL cleans up before exit */
    atexit(SDL_Quit);

    /* Initialise the screen */
    SDL_Surface* screen = SDL_SetVideoMode(vinfo->current_w, vinfo->current_h,
        32, SDL_HWSURFACE | SDL_FULLSCREEN | SDL_GL_DOUBLEBUFFER);
    if (!screen) {
        printf("Unable to set %dx%d video: %s\n",
            vinfo->current_w, vinfo->current_h, SDL_GetError());
        return 1;
    }

    /* Initialise the context */
    memset(&context, 0, sizeof(context));
    if (!context_initialize(&context, MAZE_WIDTH, MAZE_HEIGHT,
            IMAGE_WIDTH, IMAGE_HEIGHT)) {
        printf("Unable to initialise context.\n");
        return 1;
    }

    done = 0;
    while (!done) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            /* Exit if the window is closed */
            case SDL_QUIT:
                done = 1;
                break;

            /* Check for keypresses */
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    done = 1;
                    break;

                /* Prevent compiler warning */
                default: break;
                }
                break;

            /* Prevent compiler warning */
            default: break;
            }
        }

        do_display();
    }

    context_free(&context);

    return 0;
}
