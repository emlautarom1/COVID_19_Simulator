#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define WIN_W 640
#define WIN_H 640
#define CELL_SIZE 20

#define DISSEASE_STRENGTH 2.4

typedef enum Gender
{
    MALE,
    FEMALE
} Gender;

typedef enum Age
{
    CHILD,
    ADULT,
    ELDER
} Age;

typedef enum CellStatus
{
    EMPTY_WHITE = 0xFFFFFF,
    susceptible_BLUE = 0x0000FF,
    SICK_NC_ORANGE = 0xFFAA00,
    SICK_C_RED = 0xFF0000,
    ISOLATED_YELLOW = 0xFFFF00,
    CURED_GREEN = 0x00FF00,
    DEAD_BLACK = 0x000000
} CellStatus;

typedef struct Cell
{
    Age age;
    bool risk_disease;
    bool risk_job;
    bool vaccinated;
    Gender gender;
    CellStatus status;
    int contagion_t;
} Cell;

void neighbors(Cell *matrix, int matrix_w, int matrix_h, int cell_x, int cell_y, Cell *out_buffer)
{
    assert(matrix != NULL);
    assert(out_buffer != NULL);
    assert(cell_x >= 0 && cell_x < matrix_w);
    assert(cell_y >= 0 && cell_y < matrix_h);

    /*
        C is the current Cell given in (x,y)
        It's neighbors are:
        ┌───┬───┬───┐
        │ 0 │ 1 │ 2 │
        │ 3 │ C │ 4 │
        │ 5 │ 6 │ 7 │
        └───┴───┴───┘  
    */

    out_buffer[0] = matrix[((cell_y - 1 + matrix_h) % matrix_h) * matrix_w + (cell_x - 1 + matrix_w) % matrix_w];
    out_buffer[1] = matrix[((cell_y - 1 + matrix_h) % matrix_h) * matrix_w + (cell_x - 0 + matrix_w) % matrix_w];
    out_buffer[2] = matrix[((cell_y - 1 + matrix_h) % matrix_h) * matrix_w + (cell_x + 1 + matrix_w) % matrix_w];
    out_buffer[3] = matrix[((cell_y - 0 + matrix_h) % matrix_h) * matrix_w + (cell_x - 1 + matrix_w) % matrix_w];
    out_buffer[4] = matrix[((cell_y + 0 + matrix_h) % matrix_h) * matrix_w + (cell_x + 1 + matrix_w) % matrix_w];
    out_buffer[5] = matrix[((cell_y + 1 + matrix_h) % matrix_h) * matrix_w + (cell_x - 1 + matrix_w) % matrix_w];
    out_buffer[6] = matrix[((cell_y + 1 + matrix_h) % matrix_h) * matrix_w + (cell_x + 0 + matrix_w) % matrix_w];
    out_buffer[7] = matrix[((cell_y + 1 + matrix_h) % matrix_h) * matrix_w + (cell_x + 1 + matrix_w) % matrix_w];
}

bool is_sick(Cell target)
{
    return (target.status == SICK_NC_ORANGE ||
            target.status == SICK_C_RED ||
            target.status == ISOLATED_YELLOW);
}

int susceptibility(Cell target)
{
    int by_age;
    switch (target.age)
    {
    case CHILD:
        by_age = 30;
        break;
    case ADULT:
        by_age = 50;
        break;
    case ELDER:
        by_age = 90;
        break;
    default:
        break;
    }
    int by_risk = target.risk_disease || target.risk_job;

    return (by_age + by_risk) / 100;
}

int infected_neighbors(Cell *neighbors)
{
    assert(neighbors != NULL);
    int infected_count = 0;
    for (int i = 0; i < 8; i++)
    {
        if (neighbors[i].status == SICK_C_RED)
            infected_count += 1;
    }
    return infected_count;
}

void susceptible_to_sick_rule(Cell *target, Cell *neighbors, int time)
{
    assert(neighbors != NULL);
    double perc_infected_neighbors = infected_neighbors(neighbors) / 8;
    double perc_susceptibility = susceptibility(*target);
    double get_sick_chance = (perc_infected_neighbors * DISSEASE_STRENGTH) + perc_susceptibility;

    if (((rand() % 100) / 100) < get_sick_chance)
    {
        target->status = SICK_NC_ORANGE;
        target->contagion_t = time;
    }
}

void sick_to_contagious_rule(Cell *target, int time)
{
    int elapsed = time - target->contagion_t;
    if (elapsed == 4)
        target->status = SICK_C_RED;
}

void contagious_to_isolated_rule(Cell *target, int time)
{
    int elapsed = time - target->contagion_t;
    if (elapsed == 2)
    {
        int isolation_chance = 90;
        if ((rand() % 100) < isolation_chance)
            target->status = ISOLATED_YELLOW;
    }
}

void live_or_die_rule(Cell *target)
{
    double by_age;
    switch (target->age)
    {
    case CHILD:
        by_age = 1;
        break;
    case ADULT:
        by_age = 1.3;
        break;
    case ELDER:
        by_age = 14.8;
        break;
    default:
        break;
    }
    double vaccines = target->vaccinated ? 0.5 : 0;

    double death_chance = by_age - vaccines;

    if ((rand() % 100) < death_chance)
        target->status = DEAD_BLACK;
    else
        target->status = CURED_GREEN;
}

void update_cell_status(Cell *target, int time)
{
    assert(target != NULL);
    assert(time > 0);

    if (target->status == susceptible_BLUE)
        susceptible_to_sick_rule(target, /*TODO: Neighbors */ NULL, time);

    if (target->status == SICK_NC_ORANGE)
        sick_to_contagious_rule(target, time);

    if (target->status == SICK_C_RED)
        contagious_to_isolated_rule(target, time);

    if (is_sick(*target) && (time - target->contagion_t) == 14)
        live_or_die_rule(target);
}

Cell *make_cell_matrix(int w, int h)
{
    Cell *matrix = malloc((size_t)w * (size_t)h * sizeof(Cell));

    Cell empty_cell = {.status = DEAD_BLACK};
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            int pos = i * w + j;
            matrix[pos] = empty_cell;
        }
    }

    return matrix;
}

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
    srand((unsigned int)time(NULL));

    int cell_cols = WIN_W / CELL_SIZE;
    int cell_rows = WIN_H / CELL_SIZE;
    Cell *cell_matrix = make_cell_matrix(cell_cols, cell_rows);

    SDL_Rect rect = {.x = 0, .y = 0, .w = CELL_SIZE, .h = CELL_SIZE};
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
                    simulation_speed = MAX(simulation_speed - 5, 0);
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
                CellStatus status = cell_matrix[i * cell_cols + j].status;
                rect.x = j * CELL_SIZE;
                rect.y = i * CELL_SIZE;

                SDL_SetRenderDrawColor(rend,
                                       (status >> 16) & 0xFF,
                                       (status >> 8) & 0xFF,
                                       status & 0xFF,
                                       255);
                SDL_RenderFillRect(rend, &rect);
            }
        }

        // Show to screen
        SDL_RenderPresent(rend);

        SDL_Delay(1000 / 60);
    }

    // Cleanup
    free(cell_matrix);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
