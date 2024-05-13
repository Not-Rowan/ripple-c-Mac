#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <SDL2/SDL.h>

// if this is enabled (1) the ripple will be drawn when the mouse is dragged, otherwise (0) it only draws on mouse left click
#define DRAG_ENABLED 1

struct rippleParams {
    int x;
    int y;
    SDL_Surface *window_surface;
};
typedef struct rippleParams rippleParams_t;

SDL_Window *create_window(const char *window_title, int window_pos_x, int window_pos_y, int window_size_x, int window_size_y, int flags);
SDL_Surface *create_window_surface(SDL_Window *window);
void *drawRipple(rippleParams_t *params);
void drawCircle(SDL_Surface *surface, int x, int y, int radius, int blackOrWhite);
void *updateWindowSurface(void *window);

int SCREEN_WIDTH = 640; // 1280 // 640
int SCREEN_HEIGHT = 480; // 720 // 480
int is_game_running = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // mutex to lock the window surface

int main() {
    // create a window
    SDL_Window *window = create_window("ripple", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    SDL_Surface *window_surface = create_window_surface(window);

    if (pthread_mutex_init(&lock, NULL) != 0) { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    }

    pthread_t updateWindowThread;
    pthread_create(&updateWindowThread, NULL, (void *)&updateWindowSurface, (void *)window);

    is_game_running = 1;

    while (is_game_running == 1) {
        SDL_Event e;
        SDL_PollEvent(&e);
        if (e.type == SDL_QUIT) {
            is_game_running = 0;
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                //printf("Left mouse button clicked at X%d Y%d\n", e.button.x, e.button.y);
                rippleParams_t ripple;
                ripple.x = e.button.x;
                ripple.y = e.button.y;
                ripple.window_surface = window_surface;

                pthread_t thread;
                pthread_create(&thread, NULL, (void *)&drawRipple, (void *)&ripple);
            }
        } else if (e.type == SDL_MOUSEMOTION && DRAG_ENABLED == 1) {
            //printf("Left mouse button clicked at X%d Y%d\n", e.button.x, e.button.y);
            rippleParams_t ripple;
            ripple.x = e.button.x;
            ripple.y = e.button.y;
            ripple.window_surface = window_surface;

            pthread_t thread;
            pthread_create(&thread, NULL, (void *)&drawRipple, (void *)&ripple);
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    pthread_mutex_destroy(&lock);

    return 0;
}

// sudo gcc main.c -o main -Wall -F/Library/Frameworks -framework SDL2 -rpath /Library/Frameworks && ./main

/*
 ** Description
 * Constantly updates the window surface when put in a thread
 ** Parameters
 * window: the window to update the surface of
 ** Return Value
 * none
*/
void *updateWindowSurface(void *window) {
    while (is_game_running == 1) {
        pthread_mutex_lock(&lock); 
        SDL_UpdateWindowSurface(window);
        pthread_mutex_unlock(&lock); 
        SDL_Delay(16);
    }
    return NULL;
}

/*
 ** Description
 * draws a ripple starting at the given x and y position with the given radius
 ** Parameters
 * params: a struct containing the x and y position of the ripple and the window surface to draw on
 ** Return Value
 * none
*/
void *drawRipple(rippleParams_t *params) {
    int x = params->x;
    int y = params->y;
    SDL_Surface *window_surface = params->window_surface;
    
    // animate until the radius of the ripple
    for (int i = 0; i < 200; i++) {
        // draw the ripple (new ripple and erase old one)
        drawCircle(window_surface, x, y, i-1, 0);
        drawCircle(window_surface, x, y, i, 1);
        SDL_Delay(16);
    }   

    return NULL; 
}

/*
 ** Description
 * Draws a circle on a surface at the given x and y position with the given radius
 ** Parameters
 * surface: the surface to draw the circle on
 * x: the x position to start the circle
 * y: the y position to start the circle
 * radius: the radius of the circle
 * blackOrWhite: 1 for white circle, 0 for black circle
 ** Return Value
 * none
*/
void drawCircle(SDL_Surface *surface, int x, int y, int radius, int blackOrWhite) {
    // get the pixels of the surface
    Uint32 *pixels = (Uint32 *)surface->pixels;

    // for each degree in a circle, draw a pixel at the radius
    for (int i = 0; i < 360; i++) {
        int dx = radius * cos(i);
        int dy = radius * sin(i);

        // erase edges that go outside the window
        if (x + dx < 0 || x + dx >= surface->w || y + dy < 0 || y + dy >= surface->h) {
            continue;
        }
        // make the pixels slowly fade out as they get further from the center
        Uint8 r, g, b;
        if (blackOrWhite == 1) {
            r = 255 - (255 * (radius / 200.0));
            g = 255 - (255 * (radius / 200.0));
            b = 255 - (255 * (radius / 200.0));
        } else {
            r = 0;
            g = 0;
            b = 0;
        }
        // draw the pixel
        pixels[(y + dy) * surface->w + (x + dx)] = SDL_MapRGB(surface->format, r, g, b);
    }
}


/*
 ** Description
 * Creates a window to be displayed on the screen
 ** Parameters
 * window_title: the title that is displayed on the menu bar of the window
 * window_pos_x: the x position on the screen to display the window
 * window_pos_y: the y position on the screen to display the window
 * window_size_x: the x size of the displayed window
 * window_size_y: the y size of the displayed window
 * flags: additional flags for things like look of window, functionality, etc...
 ** Return Value
 * window: returns a pointer to the window that has been displayed or null if error
 */
SDL_Window *create_window(const char *window_title, int window_pos_x, int window_pos_y, int window_size_x, int window_size_y, int flags) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(stderr, "Failed to initialize the SDL2 library\n");
        return NULL;
    }

    SDL_Window *window = SDL_CreateWindow(window_title, window_pos_x, window_pos_y, window_size_x, window_size_y, flags);

    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        return NULL;
    }

    return window;
}


/*
 ** Description
 * Creates the surface of the window so the window can actually be interacted with
 ** Parameters
 * window: the window variable returned by createWindow();
 ** Return Value
 * window_surface: pointer to the window surface or null if error
 */
SDL_Surface *create_window_surface(SDL_Window *window) {
    SDL_Surface *window_surface = SDL_GetWindowSurface(window);

    if (!window_surface) {
        fprintf(stderr, "Failed to get the surface from the window\n");
        return NULL;
    }

    return window_surface;
}