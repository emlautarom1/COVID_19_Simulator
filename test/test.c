#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdbool.h>
#include <stddef.h>

#define P_PER_PROC 2

typedef enum Gender
{
    MALE = 0xFFFFFF,
    FEMALE = 0xFFFFFE
} Gender;

typedef struct Person
{
    bool alive;
    Gender gender;
    int district;
} Person;

void print_person(Person p)
{
    char show_true[] = "true";
    char show_false[] = "false";
    char show_male[] = "MALE";
    char show_female[] = "FEMALE";

    printf("Alive:    %s\n", p.alive ? show_true : show_false);
    printf("Gender:   %s\n", p.gender == MALE ? show_male : show_female);
    printf("District: %d\n\n", p.district);
}

void print_int_array(int *a, int size)
{
    printf("[");
    if (size > 0)
        printf("%d", a[0]);
    for (int i = 1; i < size; i++)
    {
        printf(",%d", a[i]);
    }
    printf("]\n");
}

void print_int_matrix(int *m, int w, int h)
{
    printf("┌");
    for (int i = 0; i < w * 3; i++)
    {
        printf("─");
    }
    printf("┐\n");

    for (int i = 0; i < h; i++)
    {
        printf("│");
        for (int j = 0; j < w; j++)
        {
            printf("%2d ", m[i * w + j]);
        }
        printf("│\n");
    }
    printf("└");
    for (int i = 0; i < w * 3; i++)
    {
        printf("─");
    }
    printf("┘\n");
}

void custom_data_types(int nprocs, int rank)
{
    MPI_Datatype MPI_MY_PERSON;
    int mpi_person_block_lengths[] = {1, 1, 1};
    MPI_Aint mpi_person_displacements[] = {
        offsetof(Person, alive),
        offsetof(Person, gender),
        offsetof(Person, district)};
    MPI_Datatype mpi_person_lengths[] = {MPI_C_BOOL, MPI_INT, MPI_INT};
    MPI_Type_create_struct(
        3, // Number of fields in 'Person'
        mpi_person_block_lengths,
        mpi_person_displacements,
        mpi_person_lengths,
        &MPI_MY_PERSON);
    MPI_Type_commit(&MPI_MY_PERSON);

    Person *everyone;
    Person *my_people = malloc((size_t)P_PER_PROC * sizeof(Person));
    for (int i = 0; i < 2; i++)
    {
        Person new_person = {.alive = true, .gender = MALE, .district = rank};
        my_people[i] = new_person;
    }
    if (rank == 0)
    {
        everyone = malloc((size_t)(nprocs * P_PER_PROC) * sizeof(Person));
    }
    MPI_Gather(my_people, P_PER_PROC, MPI_MY_PERSON, everyone, P_PER_PROC, MPI_MY_PERSON, 0, MPI_COMM_WORLD);
    if (rank == 0)
    {
        for (int i = 0; i < (P_PER_PROC * nprocs); i++)
            print_person(everyone[i]);
    }
}

void busy_waiting(int n)
{
    for (int i = 0; i < n * 9999999; i += 2)
        i--;
}

void init_int_matrix(int *m, int w, int h)
{
    for (int i = 0; i < h; i++)
        for (int j = 0; j < w; j++)
            m[i * w + j] = i * w + j;
}

void fill_int_matrix_with(int *m, int w, int h, int e)
{
    for (int i = 0; i < h; i++)
        for (int j = 0; j < w; j++)
            m[i * w + j] = e;
}

void int_circular_neighbors(int *m, int w, int h, int x, int y, int **out_b)
{
    out_b[0] = &m[((y - 1 + h) % h) * w + (x - 1 + w) % w];
    out_b[1] = &m[((y - 1 + h) % h) * w + (x - 0 + w) % w];
    out_b[2] = &m[((y - 1 + h) % h) * w + (x + 1 + w) % w];
    out_b[3] = &m[((y - 0 + h) % h) * w + (x - 1 + w) % w];
    out_b[4] = &m[((y + 0 + h) % h) * w + (x + 1 + w) % w];
    out_b[5] = &m[((y + 1 + h) % h) * w + (x - 1 + w) % w];
    out_b[6] = &m[((y + 1 + h) % h) * w + (x + 0 + w) % w];
    out_b[7] = &m[((y + 1 + h) % h) * w + (x + 1 + w) % w];
}

void padded_neighbors(void)
{
    /*
        Given a Matrix M ((m + 2) x n)
        Access each cell neighbor:
        ~ The first and last index are not used
        ? Use a displacement for accessing? Ex: (i, j) -> (i + n, j)

            Row IDX
                    ┌───┬───┬───┐
               0    │ 6 │ 7 │ 8 │ <- row 3
               1    │ 0 │ 1 │ 2 │ <┐
               2    │ 3 │ 4 │ 5 │  │ 3 rows 
               3    │ 6 │ 7 │ 8 │ <┘
               4    │ 0 │ 1 │ 2 │ <- row 1
                    └───┴───┴───┘
    */
    int m = 3;
    int n = 3;
    int *padded_matrix = malloc((size_t)((m + 2) * n) * sizeof(int));
    for (int i = 0; i < m + 2; i++)
        for (int j = 0; j < n; j++)
            padded_matrix[i * n + j] = ((i * n) + j + 6) % (m * n);

    print_int_matrix(&padded_matrix[n], n, m);

    int x = 0;
    int y = 2;
    int *neighbors[8];

    int_circular_neighbors(
        &padded_matrix[n], // Ignore the padding row
        n,                 // Respect the original width
        m,                 // and original heigth
        x + n,             // Offset the 'x' position by the first padding row
        y,                 // Respect the 'y' position
        neighbors);
    printf("(%d, %d) neighboors: ", x, y);
    for (int i = 0; i < 8; i++)
        printf("%d ", (*neighbors[i]));
    printf("\n");
}

void sending_frontiers(int nprocs, int rank)
{
    /*
        Given a Matrix M (m x n)
        Given NP, number of available procs
        * Assume 
            m % NP == 0
            m >= NP
        
        -> Each proc will handle (m / NP) + 2 rows in a circular fashion

        Example:
            3 rows, 3 cols, 3 procs
    
                * Replicate the last row at the start and the first one at the end
                * for easier distribution
    
            Row IDX
                    ┌───┬───┬───┐
               0    │ 6 │ 7 │ 8 │ <- row 3
               1    │ 0 │ 1 │ 2 │ <┐
               2    │ 3 │ 4 │ 5 │  │ 3 rows 
               3    │ 6 │ 7 │ 8 │ <┘
               4    │ 0 │ 1 │ 2 │ <- row 1
                    └───┴───┴───┘
            
            (3 / 3) + 2 rows = 3 rows/proc
            
            Rows indices/proc:
                - Proc[0] = [0,1,2] = [[6,7,8], [0,1,2], [3,4,5]]
                - Proc[1] = [1,2,3] = [[0,1,2], [3,4,5], [6,7,8]]
                - Proc[3] = [2,3,4] = [[3,4,5], [6,7,8], [0,1,2]]
        
        Example 2:
            6 rows, 6 cols, 3 procs
            
            (6 / 3) + 2 rows = 4 rows/proc
            Rows indices/proc:
                - Proc[0] = [0,1,2,3]
                - Proc[1] = [2,3,4,5]
                - Proc[3] = [4,5,6,7]
    */
    int rows = 6;
    int cols = 6;
    int padding = 2;

    if (rows % nprocs != 0)
    {
        if (rank == 0)
            printf("[DBG] Invalid number of procs: %d", nprocs);
        return;
    }
    int rows_per_proc = (rows / nprocs) + padding;
    int *my_rows = malloc((size_t)(rows_per_proc * cols) * sizeof(int));

    int *matrix;
    int *upd_matrix;
    int *sendcounts;
    int *recvcounts;
    int *displacements;

    if (rank == 0)
    {
        printf("Rows/proc: %d\n", rows_per_proc);

        // Init the matrix
        matrix = malloc((size_t)((rows + padding) * cols) * sizeof(int));
        upd_matrix = malloc((size_t)((rows + padding) * cols) * sizeof(int));
        init_int_matrix(&matrix[cols], cols, rows);

        // Copy the frontiers
        // Last to first
        memcpy(matrix, &matrix[rows * cols], (size_t)cols * sizeof(int));
        // First to last
        memcpy(&matrix[(rows + 1) * cols], &matrix[cols], (size_t)cols * sizeof(int));

        printf("Matrix:\n");
        print_int_matrix(matrix, cols, rows + padding);

        // Calculate rows indices per proc (NOT NECCESARY)
        int *row_indices = malloc((size_t)(nprocs * rows_per_proc) * sizeof(int));
        for (int i = 0; i < nprocs; i++)
        {
            for (int j = 0; j < rows_per_proc; j++)
                row_indices[i * rows_per_proc + j] = i * (rows_per_proc - padding) + j;
        }
        printf("Row indices/proc:\n");
        print_int_matrix(row_indices, rows_per_proc, nprocs);

        sendcounts = malloc((size_t)nprocs * sizeof(int));
        recvcounts = malloc((size_t)nprocs * sizeof(int));
        displacements = malloc((size_t)nprocs * sizeof(int));
        for (int i = 0; i < nprocs; i++)
        {
            // All procs work with the same number of rows
            sendcounts[i] = rows_per_proc * cols;
            recvcounts[i] = (rows_per_proc - padding) * cols;
            displacements[i] = i * (rows_per_proc - padding) * cols;
        }

        printf("Displacements: ");
        print_int_array(displacements, nprocs);
        printf("\n");
    }

    // Send to each proc the appropiate rows
    MPI_Scatterv(
        matrix,
        sendcounts,
        displacements,
        MPI_INT,
        my_rows,
        rows_per_proc * cols,
        MPI_INT,
        0,
        MPI_COMM_WORLD);

    // Delay printing
    busy_waiting(rank);

    // Work on my_rows
    fill_int_matrix_with(&my_rows[cols], cols, rows_per_proc - padding, rank);

    busy_waiting(rank);

    printf("Rank %d matrix:\n", rank);
    print_int_matrix(my_rows, cols, rows_per_proc);

    MPI_Gatherv(
        &my_rows[cols],
        (rows_per_proc - padding) * cols,
        MPI_INT,
        &upd_matrix[cols],
        recvcounts,
        displacements,
        MPI_INT,
        0,
        MPI_COMM_WORLD);

    if (rank == 0)
    {
        // Again, copy the frontiers
        // Last to first
        memcpy(upd_matrix, &upd_matrix[rows * cols], (size_t)cols * sizeof(int));
        // First to last
        memcpy(&upd_matrix[(rows + 1) * cols], &upd_matrix[cols], (size_t)cols * sizeof(int));

        busy_waiting(nprocs);
        printf("Updated Matrix:\n");
        print_int_matrix(upd_matrix, cols, rows + padding);
    }
}

int main(void)
{
    int nprocs, rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
        printf("[DBG] Running tests with %d procs...\n\n", nprocs);

    // custom_data_types(nprocs, rank);
    sending_frontiers(nprocs, rank);
    // if (rank == 0)
    //     padded_neighbors();

    MPI_Finalize();
    return 0;
}
