#include <stdio.h>
#include <math.h>
#include <mpi.h>

int MPI_BinomialBCast(void *buff, int count, MPI_Datatype datatype, MPI_Comm comm) {

    int rank, numprocs, errono, dest, dist = 1;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &numprocs);

    while (dist < numprocs){
        if(rank < dist && (rank+dist) < numprocs){
            dest = rank + dist;
            errono = MPI_Send(buff, count , datatype, dest, 0, comm);
            if(errono != MPI_SUCCESS) return errono;
        }else if(rank >= dist && rank < (dist * 2)){
            dest = rank - dist;
            errono = MPI_Recv(buff, count, datatype, dest, 0, comm, MPI_STATUS_IGNORE);
            if(errono != MPI_SUCCESS) return errono;
        }
        
        dist *= 2;
    }
    
    return MPI_SUCCESS;
}



int MPI_FlattreeColectiva( void * buff , void * recvbuff , int count , MPI_Datatype datatype, int root , MPI_Comm comm){

    int numprocs, rank, i, errono;
    double aux;

    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    //Asumimos que si no le pasamos un doble devuelve un error
    if(datatype != MPI_DOUBLE) return MPI_ERR_TYPE;
    if(comm != MPI_COMM_WORLD) return MPI_ERR_COMM;
    if(count == 0) return MPI_ERR_COUNT;
    if(buff == NULL) return MPI_ERR_BUFFER;

    //Aqui si es double hace esta función, si queremos admitir otros MPI_DATATYPE, bastaría con añadir un switch
    //En que cada caso sea un tipo de dato MPI y la función a realizar con ese datatype 
    //Asumiendo también root = 0
    if(!rank){
        for (i = 1; i < numprocs; i++) {
            aux += *(double*) buff;
            errono = MPI_Recv((double *) buff,1,MPI_DOUBLE,i,0,comm,MPI_STATUS_IGNORE);
        }
        aux += *(double*) buff;
    } else errono = MPI_Send(buff,count,datatype,0,0,comm);

    *(double*)recvbuff = aux;
    
    return errono;
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
        } 
        
        MPI_BinomialBCast(&n,1,MPI_INT,MPI_COMM_WORLD);

        if (!n) break;

        h   = 1.0 / (double) n;
        sum = 0.0;
        for (i = rank+1; i <= n; i+=numprocs) {
            x = h * ((double)i - 0.5);
            sum += 4.0 / (1.0 + x*x);
        }
        pi = (h * sum);
        
        MPI_FlattreeColectiva(&pi,&piF,1,MPI_DOUBLE,0,MPI_COMM_WORLD);

        if(!rank) printf("pi is approximately %.16f, Error is %.16f\n", piF, fabs(piF - PI25DT));
    }

    MPI_Finalize();
}

