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
#define SIM_LIMIT 120

#include "utils.h"
#include "simulation.h"

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <rows> <cols>", argv[0]);
        return -1;
    }
    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);

    if (rows < 2 || cols < 2)
    {
        fprintf(stderr, "[ERR] At least 2 rows and cols, got %d and %d", rows, cols);
        return -1;
    }

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

    Cell *matrix = malloc((size_t)(cols * rows) * sizeof(Cell));
    Cell *upd_matrix = malloc((size_t)(cols * rows) * sizeof(Cell));
    Cell *buff_neighbors[8];

    init_cell_matrix(matrix, cols, rows);

    Uint32 sim_speed = 10;
    for (int sim_t = 0; sim_t < SIM_LIMIT; sim_t++)
    {
        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                sim_t = SIM_LIMIT;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.scancode == SDL_SCANCODE_RIGHTBRACKET)
                    sim_speed = MIN(MAX_SPEED, sim_speed + 1);
                if (event.key.keysym.scancode == SDL_SCANCODE_LEFTBRACKET)
                    sim_speed = MAX(sim_speed - 1, 1);
                if (event.key.keysym.scancode == SDL_SCANCODE_Q)
                    sim_t = SIM_LIMIT;
            default:
                break;
            }
        }

        // Rendering
        SDL_Rect rect = {.w = CELL_SIZE, .h = CELL_SIZE};
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

        // Debugging
        DEBUG_PRINT("\n\tTime: %d\n\tSpeed: %d\n", sim_t, sim_speed);

        SDL_Delay(1000 / sim_speed);
    }

    DEBUG_PRINT("Simulation finished!\n");

    // Cleanup
    free(matrix);
    free(upd_matrix);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
