#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define WIN_W 640
#define WIN_H 640

#define CELL_SZ 20

typedef struct RGB
{
    Uint8 red;
    Uint8 blue;
    Uint8 green;
} RGB;

const RGB white = {.red = 255, .blue = 255, .green = 255};
const RGB blue = {.red = 0, .blue = 255, .green = 0};
const RGB orange = {.red = 240, .blue = 120, .green = 0};
const RGB red = {.red = 255, .blue = 0, .green = 0};
const RGB yellow = {.red = 255, .blue = 255, .green = 0};
const RGB green = {.red = 0, .blue = 0, .green = 255};
const RGB black = {.red = 0, .blue = 0, .green = 0};

typedef enum Gender
{
    MALE,
    FEMALE
} Gender;

typedef struct Cell
{
    int age;
    bool risk_disease;
    bool risk_job;
    bool vaccinated;
    Gender gender;
    RGB status;
} Cell;

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

    SDL_Rect rect = {.x = 0, .y = 0, .w = CELL_SZ, .h = CELL_SZ};
    const RGB colors[] = {white, blue, orange, red, yellow, green, black};

    int cell_cols = WIN_W / CELL_SZ;
    int cell_rows = WIN_H / CELL_SZ;
    int cell_count = cell_cols * cell_rows;
    Cell *cell_matrix = malloc((size_t)cell_count * sizeof(Cell));
    Cell sample = {.age = 0,
                   .risk_disease = false,
                   .risk_job = false,
                   .vaccinated = true,
                   .gender = MALE,
                   .status = white};

    for (int i = 0; i < cell_rows; i++)
    {
        for (int j = 0; j < cell_cols; j++)
        {
            int pos = i * cell_cols + j;
            cell_matrix[pos] = sample;
            cell_matrix[pos].status = colors[pos % 7];
        }
    }

    int should_exit = 0;
    int simulation_speed = 10;
    int paused = 0;
    while (!should_exit)
    {
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
                    simulation_speed += 5;
                if (event.key.keysym.scancode == SDL_SCANCODE_LEFTBRACKET)
                {
                    simulation_speed = MAX(simulation_speed - 5, 0);
                }
            default:
                break;
            }
        }
        if (paused)
            continue;

        SDL_RenderClear(rend);
        for (int i = 0; i < cell_rows; i++)
        {
            for (int j = 0; j < cell_cols; j++)
            {
                RGB status = cell_matrix[i * cell_cols + j].status;
                rect.x = j * CELL_SZ;
                rect.y = i * CELL_SZ;

                SDL_SetRenderDrawColor(rend,
                                       status.red,
                                       status.blue,
                                       status.green,
                                       255);
                SDL_RenderFillRect(rend, &rect);
            }
        }

        // Reset draw color
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);

        // Show to screen
        SDL_RenderPresent(rend);

        SDL_Delay(1000 / 60);
    }

    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
