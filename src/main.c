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

#define DEBUG true

#if defined(DEBUG) && DEBUG
#define DEBUG_PRINT(fmt, args...) printf("[DBG]: %s:%d:%s(): " fmt, \
                                         __FILE__, __LINE__, __func__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#define WIN_W 600
#define WIN_H 600
#define CELL_SIZE 10

#define MAX_SPEED 30

#define DISSEASE_STRENGTH 2.4

typedef enum Gender
{
    MALE = 0,
    FEMALE = 1
} Gender;

typedef enum Age
{
    CHILD = 0,
    ADULT = 1,
    ELDER = 2
} Age;

typedef enum CellStatus
{
    EMPTY_WHITE = 0xFFFFFF,
    SUSC_BLUE = 0x0000FF,
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
    int by_age = 0;
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
    int by_risk = (target.risk_disease || target.risk_job) ? 15 : 0;

    return by_age + by_risk;
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
    int inf_n = infected_neighbors(neighbors);
    if (inf_n == 0)
    {
        // Can't get sick if there are no infected neighbors
        return;
    }
    int susc = susceptibility(*target);
    double get_sick_chance = ((inf_n / 8) * DISSEASE_STRENGTH) + (susc / 100);

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
    double by_age = 0;
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

void new_random_alive_cell(Cell *c)
{
    Age age;
    int r = rand() % 100;
    if (r < 30)
        age = CHILD;
    else if (r >= 30 && r < 84)
        age = ADULT;
    else
        age = ELDER;

    CellStatus initial_s = rand() % 1000 < 2 ? SICK_NC_ORANGE : SUSC_BLUE;

    c->age = age,
    c->risk_disease = (rand() % 100 < 10),
    c->risk_job = (rand() % 100 < 10),
    c->vaccinated = (rand() % 100 < 70),
    c->gender = rand() % 2,
    c->status = initial_s,
    c->contagion_t = 0;
}

void init_cell_matrix(Cell *matrix, int w, int h)
{
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            int pos = i * w + j;
            if (rand() % 100 < 50)
                matrix[pos].status = EMPTY_WHITE;
            else
            {
                new_random_alive_cell(&matrix[pos]);
            }
        }
    }
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

    int cols = WIN_W / CELL_SIZE;
    int rows = WIN_H / CELL_SIZE;

    Cell *matrix = malloc((size_t)(cols * rows) * sizeof(Cell));
    Cell *upd_matrix = malloc((size_t)(cols * rows) * sizeof(Cell));
    Cell *buff_neighbors = malloc(8 * sizeof(Cell));

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
        DEBUG_PRINT("\n\tTime: %d\n\tSpeed: %d\n", sim_t, sim_speed);
        SDL_Delay(1000 / sim_speed);
    }

    // Cleanup
    free(buff_neighbors);
    free(matrix);
    free(upd_matrix);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
