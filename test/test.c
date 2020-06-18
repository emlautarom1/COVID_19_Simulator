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

int main(void)
{
    int nprocs, rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
        printf("[DBG] Running tests with %d procs...\n\n", nprocs);

    // custom_data_types(nprocs, rank);
    // sending_frontiers(nprocs, rank);

    MPI_Finalize();
    return 0;
}
