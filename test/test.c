#include <stdio.h>
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

void circular_rows(void)
{
    /*
        Given a matrix M (m x n) - m rows, n columns
        Given NP, number of available procs
        * Assume 
            m % NP == 0
            m >= NP
        
        -> Each proc will handle (m / NP) + 2 rows in a circular fashion
        
        Ex:
            NP             = 3
            (m, n)         = (6, 6)
            Rows per proc = (6 / 3) + 2 = 2 + 2 = 4
            
            Proc[0] = [5 0 1 2]
            Proc[1] = [1 2 3 4]
            Proc[2] = [3 4 5 0]
    */
    int rows = 6;
    int nprocs = 6;
    int padding = 2;
    int rows_per_proc = (rows / nprocs) + padding;

    int *actual[nprocs];
    for (int i = 0; i < nprocs; i++)
    {
        actual[i] = malloc((size_t)rows_per_proc * sizeof(int));
        for (int j = 0; j < rows_per_proc; j++)
        {
            actual[i][j] = (i * (rows_per_proc - padding) + j - 1 + rows) % rows;
        }
    }
    for (int i = 0; i < nprocs; i++)
    {
        printf("Proc[%d]: ", i);
        print_int_array(actual[i], rows_per_proc);
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

    custom_data_types(nprocs, rank);
    if (rank == 0)
        circular_rows();

    MPI_Finalize();
    return 0;
}
