#include <stdio.h>
#include <assert.h>

#define DISSEASE_STRENGTH 2.4

extern int rand(void);

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

void neighbors(Cell *matrix, int matrix_w, int matrix_h, int cell_x, int cell_y, Cell **out_buffer)
{
    assert(matrix != NULL);
    assert(out_buffer != NULL);
    assert(cell_x >= 0);
    assert(cell_y >= 0);

    /*
        C is the current Cell given in (x,y)
        It's neighbors are:
        ┌───┬───┬───┐
        │ 0 │ 1 │ 2 │
        │ 3 │ C │ 4 │
        │ 5 │ 6 │ 7 │
        └───┴───┴───┘  
    */

    out_buffer[0] = &matrix[((cell_y - 1 + matrix_h) % matrix_h) * matrix_w + (cell_x - 1 + matrix_w) % matrix_w];
    out_buffer[1] = &matrix[((cell_y - 1 + matrix_h) % matrix_h) * matrix_w + (cell_x - 0 + matrix_w) % matrix_w];
    out_buffer[2] = &matrix[((cell_y - 1 + matrix_h) % matrix_h) * matrix_w + (cell_x + 1 + matrix_w) % matrix_w];
    out_buffer[3] = &matrix[((cell_y - 0 + matrix_h) % matrix_h) * matrix_w + (cell_x - 1 + matrix_w) % matrix_w];
    out_buffer[4] = &matrix[((cell_y + 0 + matrix_h) % matrix_h) * matrix_w + (cell_x + 1 + matrix_w) % matrix_w];
    out_buffer[5] = &matrix[((cell_y + 1 + matrix_h) % matrix_h) * matrix_w + (cell_x - 1 + matrix_w) % matrix_w];
    out_buffer[6] = &matrix[((cell_y + 1 + matrix_h) % matrix_h) * matrix_w + (cell_x + 0 + matrix_w) % matrix_w];
    out_buffer[7] = &matrix[((cell_y + 1 + matrix_h) % matrix_h) * matrix_w + (cell_x + 1 + matrix_w) % matrix_w];
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

int infected_neighbors(Cell **neighbors)
{
    assert(neighbors != NULL);
    int infected_count = 0;
    for (int i = 0; i < 8; i++)
    {
        if (neighbors[i]->status == SICK_C_RED)
            infected_count += 1;
    }
    return infected_count;
}

void susceptible_to_sick_rule(Cell *target, Cell **neighbors, int time)
{
    assert(neighbors != NULL);
    int inf_n = infected_neighbors(neighbors);
    if (inf_n == 0)
        return; // Can't get sick if there are no infected neighbors
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
    assert(target != NULL);
    int elapsed = time - target->contagion_t;
    if (elapsed == 4)
        target->status = SICK_C_RED;
}

void contagious_to_isolated_rule(Cell *target, int time)
{
    assert(target != NULL);
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
    assert(target != NULL);
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
    assert(c != NULL);

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
    c->gender = (Gender)(rand() % 2),
    c->status = initial_s,
    c->contagion_t = 0;
}

void init_cell_matrix(Cell *matrix, int w, int h)
{
    assert(matrix != NULL);
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            int pos = i * w + j;
            if (rand() % 100 < 50)
                matrix[pos].status = EMPTY_WHITE;
            else
                new_random_alive_cell(&matrix[pos]);
        }
    }
}