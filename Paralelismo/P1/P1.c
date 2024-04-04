#include <stdio.h>
#include <math.h>
#include <mpi.h>

void my_send(int var,int numprocs){
    for (int i = 1; i < numprocs; i++) {
        MPI_Send(&var,1,MPI_INT,i,0,MPI_COMM_WORLD);
    }
}

double my_recv(double var,int numprocs){
    double pi=0;
    for (int i = 1; i < numprocs; i++) {
        pi += var;
        MPI_Recv(&var,1,MPI_DOUBLE,i,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }
    pi += var;
    return pi;
}

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
            my_send(n,numprocs);
        } else MPI_Recv(&n,1,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

        if (!n) break;

        h   = 1.0 / (double) n;
        sum = 0.0;
        for (i = rank+1; i <= n; i+=numprocs) {
            x = h * ((double)i - 0.5);
            sum += 4.0 / (1.0 + x*x);
        }
        pi = (h * sum);

        if(rank) MPI_Send(&pi,1,MPI_DOUBLE,0,0,MPI_COMM_WORLD);
        
        if(!rank){
            piF = my_recv(pi,numprocs);
            printf("pi is approximately %.16f, Error is %.16f\n", piF, fabs(piF - PI25DT));
        }
    }
    MPI_Finalize();
}
