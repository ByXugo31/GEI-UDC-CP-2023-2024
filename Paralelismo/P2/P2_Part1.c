#include <stdio.h>
#include <math.h>
#include <mpi.h>

int main(int argc, char *argv[]){
    int rank, numprocs;
    int i, done = 0, n;
    double PI25DT = 3.141592653589793238462643;
    double pi, h, sum, x;
    double piF;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    while (!done) 
    {
        if(!rank){
            printf("Enter the number of intervals: (0 quits) \n");
            scanf("%d",&n);
        }

        MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);

        if (!n) break;

        h   = 1.0 / (double) n;
        sum = 0.0;
        for (i = rank+1; i <= n; i+=numprocs) {
            x = h * ((double)i - 0.5);
            sum += 4.0 / (1.0 + x*x);
        }
        pi = (h * sum);

        MPI_Reduce(&pi,&piF,1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);

        if(!rank) printf("pi is approximately %.16f, Error is %.16f\n", piF, fabs(piF - PI25DT));

    }

    MPI_Finalize();
}
