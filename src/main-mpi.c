#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <mpi.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

#define DEBUG true

#define WIN_W 600
#define WIN_H 600
#define CELL_SIZE 10

#define MASTER_RANK 0

#define SIM_LIMIT 120
#define R_PADDING 2

#include "utils.h"
#include "simulation.h"

int main(int argc, char const *argv[])
{
    int nprocs, rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <rows> <cols>", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);

    if (rows < 2 || cols < 2)
    {
        fprintf(stderr, "[ERR] At least 2 rows and cols, got %d and %d", rows, cols);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    if (rows % nprocs != 0)
    {
        fprintf(stderr, "[ERR] rows (%d) %% nprocs (%d) != 0", rows, nprocs);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    // SDL Setup
    SDL_Window *window;
    SDL_Renderer *rend;
    if (rank == MASTER_RANK)
    {
        window = SDL_CreateWindow("COVID-19 Simulator",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  WIN_W, WIN_H, 0);
        if (!window)
        {
            printf("Error creating main window: %s\n", SDL_GetError());
            SDL_Quit();
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        rend = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

        if (!rend)
        {
            printf("Error creating renderer: %s\n", SDL_GetError());
            SDL_DestroyWindow(window);
            SDL_Quit();
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }

    // Init random number generation
    srand(__rand_seed__);

    int rows_per_proc = (rows / nprocs) + R_PADDING;
    Cell *my_matrix = malloc((size_t)(rows_per_proc * cols) * sizeof(Cell));
    Cell *my_upd_matrix = malloc((size_t)(rows_per_proc * cols) * sizeof(Cell));
    Cell *my_buff_neighbors[8];

    Cell *matrix;
    Cell *upd_matrix;
    int *sendcounts;
    int *recvcounts;
    int *displacements;

    if (rank == MASTER_RANK)
        DEBUG_PRINT("Starting MPI_COVID19_CELL type registration\n");
    MPI_Datatype MPI_COVID19_CELL;
    int mpi_cell_block_lengths[] = {1, 1, 1, 1, 1, 1, 1};
    MPI_Aint mpi_cell_displacements[] = {
        offsetof(Cell, age),
        offsetof(Cell, risk_disease),
        offsetof(Cell, risk_job),
        offsetof(Cell, vaccinated),
        offsetof(Cell, gender),
        offsetof(Cell, status),
        offsetof(Cell, contagion_t)};
    MPI_Datatype mpi_cell_lengths[] = {
        MPI_INT,    // age
        MPI_C_BOOL, // risk_disease
        MPI_C_BOOL, // risk_job
        MPI_C_BOOL, // vaccinated
        MPI_INT,    // gender
        MPI_INT,    // status
        MPI_INT     // contagion_t
    };
    MPI_Type_create_struct(
        7, // Number of fields in 'Cell'
        mpi_cell_block_lengths,
        mpi_cell_displacements,
        mpi_cell_lengths,
        &MPI_COVID19_CELL);
    MPI_Type_commit(&MPI_COVID19_CELL);
    if (rank == MASTER_RANK)
        DEBUG_PRINT("MPI_COVID19_CELL type registered\n");

    if (rank == MASTER_RANK)
    {
        DEBUG_PRINT("Rows/proc: %d\n", rows_per_proc);

        // Init the matrix
        matrix = malloc((size_t)((rows + R_PADDING) * cols) * sizeof(Cell));
        upd_matrix = malloc((size_t)((rows + R_PADDING) * cols) * sizeof(Cell));
        init_cell_matrix(&matrix[cols], cols, rows);

        // Copy the frontiers
        // Last to first
        memcpy(matrix, &matrix[rows * cols], (size_t)cols * sizeof(Cell));
        // First to last
        memcpy(&matrix[(rows + 1) * cols], &matrix[cols], (size_t)cols * sizeof(Cell));

        sendcounts = malloc((size_t)nprocs * sizeof(int));
        recvcounts = malloc((size_t)nprocs * sizeof(int));
        displacements = malloc((size_t)nprocs * sizeof(int));
        for (int i = 0; i < nprocs; i++)
        {
            // All procs work with the same number of rows
            sendcounts[i] = rows_per_proc * cols;
            recvcounts[i] = (rows_per_proc - R_PADDING) * cols;
            displacements[i] = i * (rows_per_proc - R_PADDING) * cols;
        }

        DEBUG_PRINT("Master rank setup dance complete\n");
    }

    for (int sim_t = 0; sim_t < SIM_LIMIT; sim_t++)
    {
        // Rendering on master rank
        if (rank == MASTER_RANK)
        {
            SDL_Rect rect = {.w = CELL_SIZE, .h = CELL_SIZE};
            SDL_RenderClear(rend);
            for (int i = 0; i < rows; i++)
            {
                for (int j = 0; j < cols; j++)
                {
                    CellStatus current_status = matrix[cols + (i * cols) + j].status;
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
        }

        // Send to each proc the appropiate rows
        MPI_Scatterv(
            matrix,
            sendcounts,
            displacements,
            MPI_COVID19_CELL,
            my_matrix,
            rows_per_proc * cols,
            MPI_COVID19_CELL,
            0,
            MPI_COMM_WORLD);

        // Per proc processing
        memcpy(my_upd_matrix, my_matrix, (size_t)(rows_per_proc * cols) * sizeof(Cell));
        for (int i = 0; i < rows_per_proc - R_PADDING; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                Cell *current = &my_upd_matrix[cols + (i * cols) + j];
                if (current->status == SUSC_BLUE)
                {
                    neighbors(my_matrix, cols, rows_per_proc, j, i + 1, my_buff_neighbors);
                    susceptible_to_sick_rule(current, my_buff_neighbors, sim_t);
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

        // Gather the updated matrix on the master proc
        MPI_Gatherv(
            &my_upd_matrix[cols],
            (rows_per_proc - R_PADDING) * cols,
            MPI_COVID19_CELL,
            &upd_matrix[cols],
            recvcounts,
            displacements,
            MPI_COVID19_CELL,
            0,
            MPI_COMM_WORLD);

        // Pointer dance per proc
        void *my_temp = my_matrix;
        my_matrix = my_upd_matrix;
        my_upd_matrix = my_temp;

        if (rank == MASTER_RANK)
        {
            void *temp = matrix;
            matrix = upd_matrix;
            upd_matrix = temp;
        }
    }
    if (rank == MASTER_RANK)
        DEBUG_PRINT("Simulation finished!\n");

    // Cleanup
    if (rank == MASTER_RANK)
    {
        free(matrix);
        free(upd_matrix);
        free(sendcounts);
        free(recvcounts);
        free(displacements);
    }
    free(my_matrix);
    free(my_upd_matrix);

    if (rank == MASTER_RANK)
        SDL_Quit();
    MPI_Finalize();
    return 0;
}
