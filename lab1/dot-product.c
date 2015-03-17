#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

const int VECTOR_COUNT = 100000000;
const int RANK_ROOT = 0;

int main(int argc, char **argv) {
    
    // var
    double *x, *y, *localX, *localY;
    double sum = 0, localSum;
    double startTime, endTime;
    int size, rank, perSize;
    int i;

    // init MPI
    MPI_Init (&argc, &argv);

    
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    perSize = VECTOR_COUNT/size;

    // init data
    if (rank == RANK_ROOT) {
        x = (double *) malloc(VECTOR_COUNT*sizeof(double));
        y = (double *) malloc(VECTOR_COUNT*sizeof(double));
        for (i=0; i<VECTOR_COUNT; i++) {
            x[i] = 1.0;
            y[i] = 1.0;
        }
        startTime = MPI_Wtime();
    }
    
    localX = (double *) malloc(perSize*sizeof(double));
    localY = (double *) malloc(perSize*sizeof(double));

    // scatter
    MPI_Scatter(x, perSize, MPI_DOUBLE, localX, perSize, MPI_DOUBLE, RANK_ROOT, MPI_COMM_WORLD);
    MPI_Scatter(y, perSize, MPI_DOUBLE, localY, perSize, MPI_DOUBLE, RANK_ROOT, MPI_COMM_WORLD);

    // calc
    for (i=0; i<perSize; i++) {
        localSum += localX[i] * localY[i];
    }
    free(localX);
    free(localY);
    
    // reduce
    MPI_Reduce(&localSum, &sum, 1, MPI_DOUBLE, MPI_SUM, RANK_ROOT, MPI_COMM_WORLD);

    if (rank == RANK_ROOT) {
        printf("Dot = %.2lf\n", sum);
        endTime = MPI_Wtime();
        printf("Total Running Time: %.2lf\n", endTime-startTime);
    }

    MPI_Finalize();
    exit(0);
    return 0;
}

