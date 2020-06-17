#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

#define DEBUG true

#define WIN_W 600
#define WIN_H 600
#define CELL_SIZE 10

#define MAX_SPEED 30

#include "utils.h"
#include "simulation.h"

int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return -1;
    }
    SDL_Window *window = SDL_CreateWindow("COVID-19 Simulator",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WIN_W, WIN_H, 0);
    if (!window)
    {
        printf("Error creating main window: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Renderer *rend = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!rend)
    {
        printf("Error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Init random number generation
    srand(__rand_seed__);

    int cols = WIN_W / CELL_SIZE;
    int rows = WIN_H / CELL_SIZE;

    Cell *matrix = malloc((size_t)(cols * rows) * sizeof(Cell));
    Cell *upd_matrix = malloc((size_t)(cols * rows) * sizeof(Cell));
    Cell *buff_neighbors[8];

    init_cell_matrix(matrix, cols, rows);

    int sim_t = 0;
    Uint32 sim_speed = 10;

    SDL_Rect rect = {.w = CELL_SIZE, .h = CELL_SIZE};
    int should_exit = 0;
    int paused = 0;
    while (!should_exit)
    {
        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                should_exit = 1;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
                    paused = !paused;
                if (event.key.keysym.scancode == SDL_SCANCODE_RIGHTBRACKET)
                    sim_speed = MIN(MAX_SPEED, sim_speed + 1);
                if (event.key.keysym.scancode == SDL_SCANCODE_LEFTBRACKET)
                    sim_speed = MAX(sim_speed - 1, 1);
                if (event.key.keysym.scancode == SDL_SCANCODE_R)
                {
                    init_cell_matrix(matrix, cols, rows);
                    sim_t = 0;
                }
                if (event.key.keysym.scancode == SDL_SCANCODE_Q)
                    should_exit = true;
            default:
                break;
            }
        }
        if (paused)
            continue;

        // Rendering
        SDL_RenderClear(rend);
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                CellStatus current_status = matrix[i * cols + j].status;
                rect.x = j * CELL_SIZE;
                rect.y = i * CELL_SIZE;

                SDL_SetRenderDrawColor(rend,
                                       (current_status >> 16) & 0xFF,
                                       (current_status >> 8) & 0xFF,
                                       current_status & 0xFF,
                                       255);
                SDL_RenderFillRect(rend, &rect);
            }
        }
        SDL_RenderPresent(rend);

        // Update
        memcpy(upd_matrix, matrix, (size_t)(cols * rows) * sizeof(Cell));
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                Cell *current = &upd_matrix[i * cols + j];
                if (current->status == SUSC_BLUE)
                {
                    neighbors(matrix, cols, rows, j, i, buff_neighbors);
                    susceptible_to_sick_rule(current, buff_neighbors, sim_t);
                }
                if (current->status == SICK_NC_ORANGE)
                {
                    sick_to_contagious_rule(current, sim_t);
                }
                if (current->status == SICK_C_RED)
                {
                    contagious_to_isolated_rule(current, sim_t);
                }
                if (is_sick(*current) && (sim_t - current->contagion_t) == 14)
                {
                    live_or_die_rule(current);
                }
            }
        }

        void *temp = matrix;
        matrix = upd_matrix;
        upd_matrix = temp;

        sim_t++;
        // Debugging
        DEBUG_PRINT("\n\tTime: %d\n\tSpeed: %d\n", sim_t, sim_speed);

        SDL_Delay(1000 / sim_speed);
    }

    // Cleanup
    free(matrix);
    free(upd_matrix);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
