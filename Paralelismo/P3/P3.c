#include <stdio.h>
#include <sys/time.h>
#include <mpi.h>
#define DEBUG 1

#define N 1024

//FUNCIONES DE DEBUG
/*
void print_vector(int rank,int n, float v[]){
    int i;
    printf("-------%d---------\n",rank);
    for(i=0;i<n;i++) {
        printf("%f ",v[i]);
    }
    printf("\n\n");
}

void print_matrix(int rank,int n, float matrix[][N]){
    int i,j;
    printf("---------------%d-----------------\n",rank);
    for(i=0;i<n;i++) {
        for(j=0;j<N;j++) {
            printf("%f ",matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}
*/

int main(int argc, char *argv[] ) {

    int i, j, rank, numprocs, lim, elements;
    float matrix[N][N];
    float vector[N];
    float result[N];
    float global_result[N*N];
    struct timeval  tv1, tv2;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    
    elements = N/numprocs;
    if(N % numprocs != 0) elements++;

    float local_buff[elements];
    float local_matrix[elements][N];

    /* Initialize Matrix and Vector */
    if(!rank){        
        for(i=0;i<N;i++) {
            result[i] = 0;
            vector[i] = i;
            for(j=0;j<N;j++) matrix[i][j] = i+j;
        }
    }

    MPI_Scatter(matrix,elements*N,MPI_FLOAT,local_matrix,elements*N,MPI_FLOAT,0,MPI_COMM_WORLD);
    MPI_Bcast(vector,N,MPI_FLOAT,0,MPI_COMM_WORLD);
    gettimeofday(&tv1, NULL);

    for(i=0;i<elements;i++) {
        local_buff[i]=0;
        for(j=0;j<N;j++) {
            local_buff[i] += local_matrix[i][j]*vector[j];
        }
    }

    gettimeofday(&tv2, NULL);
    
    int microseconds = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

    MPI_Gather(local_buff, elements, MPI_FLOAT, result, elements, MPI_FLOAT, 0, MPI_COMM_WORLD);

    /*Display result */
    if(!rank){
        if (DEBUG){
            for(i=0;i<N;i++) printf(" %f \t ",result[i]);
        } else printf ("Time (seconds) = %lf\n", (double) microseconds/1E6);
    }
    
    MPI_Finalize();

    return 0;
}   