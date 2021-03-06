#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
    #include <SDL/SDL.h>
#else
    #include <SDL.h>
#endif

#include "context.h"

#include "arguments/arguments.h"

/**
 * The width of the stereogram image.
 */
#define IMAGE_WIDTH 512

/**
 * The height of the stereogram image.
 */
#define IMAGE_HEIGHT 512

/**
 * The number of milliseconds between each redraw.
 */
#define TIMER_INTERVAL 40

/**
 * The number of milliseconds late a render event may happen before rendering is
 * ignored for the current frame.
 */
#define TIMER_MARGIN 10

/**
 * The acceleration caused by the keys and the joystick.
 */
#define ACCELERATION 0.2

/**
 * The user event code that signals that the display should be refreshed.
 */
#define USER_EVENT_DISPLAY (SDL_USEREVENT + 1)

/**
 * Whether to prevent flooding the CPU with stereogram generation requests.
 *
 * If the CPU does not have time to render stereograms, the process may
 * eventually slow down to a crawl. If too many frames are skipped though, the
 * scene will jerk.
 */
static int prevent_flooding = 0;

/**
 * The timer callback function.
 *
 * This function pushes an event to SDL to make it execute in the main thread.
 *
 * @param interval
 *     The timer interval.
 * @param dummy
 *     Not used.
 * @return the next timer interval, which is the same as the current
 */
static Uint32
do_timer(Uint32 interval, void *dummy)
{
    SDL_Event event;

    event.user.type = SDL_USEREVENT;
    event.user.code = USER_EVENT_DISPLAY;
    event.user.data1 = NULL;
    event.user.data2 = NULL;

    SDL_PushEvent(&event);

    return interval;
}

/**
 * Updates the display.
 *
 * @param context
 *     The context.
 */
static void
do_display(Context *context)
{
    static Uint32 last_ticks = 0;
    Uint32 current_ticks = SDL_GetTicks();
    glLoadIdentity();

    /* Render the context if we have not missed the render window */
    if (!prevent_flooding || !last_ticks
            || current_ticks - last_ticks < TIMER_INTERVAL + TIMER_MARGIN) {
        context_render(context);
    }
    last_ticks = current_ticks;

    /* Update the target and camera */
    context_target_move(context);
    context_camera_move(context);

    /* Render to screen */
    SDL_GL_SwapBuffers();
}

/**
 * Handles any pending SDL events.
 *
 * @return non-zero if the application should continue running and 0 otherwise
 */
static int
handle_events(Context *context)
{
    SDL_Event event;

    while (SDL_WaitEvent(&event)) {
        switch (event.type) {
        /* Exit if the window is closed */
        case SDL_QUIT:
            return 0;

        /* Check for keypresses */
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                return 0;

            case SDLK_SPACE:
                context->gl.render_stereo = !context->gl.render_stereo;
                break;

            case SDLK_f:
                prevent_flooding = !prevent_flooding;
                break;

            case SDLK_p:
                context->stereo.update_pattern =
                    !context->stereo.update_pattern;
                break;

            case SDLK_t:
                context->gl.apply_texture = !context->gl.apply_texture;
                break;

            case SDLK_UP:
                context_target_accelerate_y(context, -ACCELERATION);
                break;

            case SDLK_DOWN:
                context_target_accelerate_y(context, ACCELERATION);
                break;

            case SDLK_LEFT:
                context_target_accelerate_x(context, -ACCELERATION);
                break;

            case SDLK_RIGHT:
                context_target_accelerate_x(context, ACCELERATION);
                break;

            /* Prevent compiler warning */
            default: break;
            }
            break;

        case SDL_KEYUP:
            switch (event.key.keysym.sym) {
            case SDLK_UP:
                if (context->target.ay < 0.0) {
                    context_target_accelerate_y(context, 0.0);
                }
                break;

            case SDLK_DOWN:
                if (context->target.ay > 0.0) {
                    context_target_accelerate_y(context, 0.0);
                }
                break;

            case SDLK_LEFT:
                if (context->target.ax < 0.0) {
                    context_target_accelerate_x(context, 0.0);
                }
                break;

            case SDLK_RIGHT:
                if (context->target.ax > 0.0) {
                    context_target_accelerate_x(context, 0.0);
                }
                break;

            /* Prevent compiler warning */
            default: break;
            }
            break;

        case SDL_JOYAXISMOTION:
            switch (event.jaxis.axis) {
            case 0:
                context_target_accelerate_x(context,
                    ACCELERATION * (double)event.jaxis.value / 32768);
                break;

            case 1:
                context_target_accelerate_y(context,
                    ACCELERATION * (double)event.jaxis.value / 32768);
                break;
            }
            break;

        case SDL_USEREVENT:
            switch (event.user.code) {
            case USER_EVENT_DISPLAY:
                do_display(context);
                break;

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

static int
main(int argc, char *argv[],
    window_size_t window_size,
    maze_size_t maze_size,
    double wall_width,
    double slope_width,
    double shortcut_ratio,
    double stereogram_strength,
    StereoPattern *pattern_image)
{
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("Unable to init SDL: %s\n", SDL_GetError());
        return 1;
    }
    atexit(SDL_Quit);

    /* Hide the mouse cursor */
    SDL_ShowCursor(0);

    /* Get video information */
    const SDL_VideoInfo *vinfo = SDL_GetVideoInfo();
    if (!vinfo) {
        printf("Unable to get video info: %s\n", SDL_GetError());
        return 1;
    }

    /* Initialise the screen */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Surface* screen;
    if (window_size.width > 0 && window_size.height > 0) {
        screen = SDL_SetVideoMode(window_size.width, window_size.height,
            32, SDL_OPENGL);
    }
    else {
        screen = SDL_SetVideoMode(vinfo->current_w, vinfo->current_h,
            32, SDL_OPENGL | SDL_FULLSCREEN);
    }
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
    if (!context_initialize(&context, IMAGE_WIDTH, IMAGE_HEIGHT,
            vinfo->current_w, vinfo->current_h, pattern_image)) {
        printf("Unable to initialise context.\n");
        return 1;
    }

    /* Zero the cached value, since the pattern now is owned by the context */
    ARGUMENT_VALUE(pattern_image) = NULL;

    /* Create the timer */
    SDL_TimerID timer = SDL_AddTimer(TIMER_INTERVAL, do_timer, NULL);
    if (!timer) {
        context_free(&context);
        printf("Unable to add timer.\n");
        return 1;
    }

    /* Open the joystick */
    SDL_Joystick *joystick = NULL;
    int jindex;
    for (jindex = 0; jindex < SDL_NumJoysticks(); jindex++) {
        joystick = SDL_JoystickOpen(jindex);
        if (joystick) {
            /* If we have at least two axes, use this joystick */
            if (SDL_JoystickNumAxes(joystick) >= 2) {
                printf("Found joystick %s\n", SDL_JoystickName(jindex));
                break;
            }

            SDL_JoystickClose(joystick);
        }
    }

    /* Enter the main loop */
    while (handle_events(&context));

    SDL_JoystickClose(joystick);

    SDL_RemoveTimer(timer);

    context_free(&context);

    return 0;
}
