#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mpi.h>

#define DEBUG true

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

        // TODO: Register Cell as a MPI type
    }

    for (int sim_t = 0; sim_t < 120; sim_t++)
    {
        // Send to each proc the appropiate rows
        MPI_Scatterv(
            matrix,
            sendcounts,
            displacements,
            MPI_INT,
            my_matrix,
            rows_per_proc * cols,
            MPI_INT,
            0,
            MPI_COMM_WORLD);

        // TODO: Do work on each proc

        // Gather the updated matrix on the master proc
        MPI_Gatherv(
            &my_matrix[cols],
            (rows_per_proc - R_PADDING) * cols,
            MPI_INT,
            &upd_matrix[cols],
            recvcounts,
            displacements,
            MPI_INT,
            0,
            MPI_COMM_WORLD);

        if (rank == MASTER_RANK)
        {
            void *temp = matrix;
            matrix = upd_matrix;
            upd_matrix = temp;

            DEBUG_PRINT("\n\tTime: %d\n", sim_t);
        }
    }

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

    MPI_Finalize();
    return 0;
}
