#include <stdio.h>
#include <sys/time.h>
#include <mpi.h>
#include <stdlib.h>
#define DEBUG 0

#define N 1024

// * : FUNCIONES DE DEBUG
/*
void print_vector(int rank,int n, float v[]){
    int i;
    printf("-------%d---------\n",rank);
    for(i=0;i<n;i++) {
        printf("%f ",v[i]);
    }
    printf("\n\n");
}

void print_matrix1(int rank,int n, float *matrix){
    int i,j;
    printf("---------------%d-----------------\n",rank);
    for(i=0;i<n;i++) {
        for(j=0;j<N;j++) {
            printf("%f ",matrix[i*N+j]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_matrix2(int rank,int n, float matrix[][N]){
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

// * : FUNCIONES AUXILIARES
void calcular_resultado(float local_buff[], float local_matrix[][N], float vector[], int elements, int start_value){
    int i,j;
    for(i=start_value;i<elements;i++) {
        local_buff[i]=0;
        for(j=0;j<N;j++) {
            local_buff[i] += local_matrix[i][j]*vector[j];
        }
    }
}

void calcular_resultado_excedente(float local_buff[], float *matrix, float vector[], int elements, int start_value){
    int i,j;
    for(i=start_value;i<elements;i++) {
        local_buff[i]=0;
        for(j=0;j<N;j++) {
            local_buff[i] += matrix[i*N+j]*vector[j];
        }
    }
}

void print_Time(int rank, char* str, struct timeval tv1, struct timeval tv2){
    int microseconds = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);
    char str2[100];
    sprintf(str2,"%d %s %s %.16lf",rank,str," = ",(double) microseconds/1E6);
    printf ("%s\n",str2);
}

int main(int argc, char *argv[] ) {

    int i, j, rank, numprocs, elements, microseconds, extra_rows;
    float vector[N];
    float *result, *matrix;
    struct timeval  tv1, tv2;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    elements = N/numprocs;
    float local_buff[elements];
    float local_matrix[elements][N];

    // *: Initialize Matrix and Vector
    if(!rank){      
        matrix = malloc(N*N*sizeof(float));
        result = malloc(N*sizeof(float));
        for(i=0;i<N;i++) {
            result[i] = 0;
            vector[i] = i;
            for(j=0;j<N;j++) matrix[i*N+j] = i+j;
        }
    }

    gettimeofday(&tv1, NULL);

    MPI_Scatter(matrix,elements*N,MPI_FLOAT,local_matrix,elements*N,MPI_FLOAT,0,MPI_COMM_WORLD);

    MPI_Bcast(vector,N,MPI_FLOAT,0,MPI_COMM_WORLD);

    gettimeofday(&tv2, NULL);

    print_Time(rank,"Scatter and Bcast Time (seconds)",tv1,tv2);

    gettimeofday(&tv1, NULL);

    calcular_resultado(local_buff,local_matrix,vector,elements,0);
    
    // *: Calculate the remaining rows
    if(!rank) {
        extra_rows = N % numprocs;
        if(extra_rows != 0) {
            calcular_resultado_excedente(result + elements * numprocs, matrix + elements * numprocs * N, vector, extra_rows, 0);
        }
    }

    gettimeofday(&tv2, NULL);
    
    print_Time(rank,"Computation Time (seconds)",tv1,tv2);

    gettimeofday(&tv1, NULL);
    
    MPI_Gather(local_buff, elements, MPI_FLOAT, result, elements, MPI_FLOAT, 0, MPI_COMM_WORLD);

    gettimeofday(&tv2, NULL);

    print_Time(rank,"Gather Time (seconds)",tv1,tv2);

    // * : Display result 
    if(!rank && DEBUG){
        printf("--------RESULT---------\n");
        for(i=0;i<N;i++) printf(" %f \t ",result[i]);
        printf("\n");
        free(matrix);
        free(result);
    }

    MPI_Finalize();

    return 0;
}   
