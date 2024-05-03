#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "compress.h"
#include "chunk_archive.h"
#include "queue.h"
#include "options.h"

#define CHUNK_SIZE (1024*1024)
#define QUEUE_SIZE 20

#define COMPRESS 1
#define DECOMPRESS 0

struct args{
    int chunks;
    int iterationsRead;
    int iterationsWrite;
    int iterationsWorker;
    int size;
    int fd;
    archive ar;
    queue in;
    queue out;
    pthread_mutex_t *mutexIterRd;
    pthread_mutex_t *mutexIterWr;
    pthread_mutex_t *mutexIterWk;
};

pthread_mutex_t *inicializarMutex(){
    pthread_mutex_t *mtx = malloc(sizeof (pthread_mutex_t));
    pthread_mutex_init(mtx,NULL);
    return mtx;
}

void destruirMutex(pthread_mutex_t *mtx){
    pthread_mutex_destroy(mtx);
    free(mtx);
}

int checkLoop(int *iters,int chunks,pthread_mutex_t *mutex){
    pthread_mutex_lock(mutex);
    if(*iters>=chunks){
        pthread_mutex_unlock(mutex);
        return -1;
    }
    (*iters)++;
    pthread_mutex_unlock(mutex);
    return 0;
}

// read input file and send chunks to the in queue
void *readChunk(void *ptr){
    struct args *args = ptr;
    chunk ch;
    int offset;
    while (1){
        if(checkLoop(&args->iterationsRead,args->chunks,args->mutexIterRd)==-1) break;
        ch = alloc_chunk(args->size);
        offset = lseek(args->fd, 0, SEEK_CUR);
        ch->size   = read(args->fd, ch->data, args->size);
        ch->num    = args->iterationsRead-1;
        ch->offset = offset;
        q_insert(args->in,ch);
    }
    return NULL;
}

// send chunks to the output archive file
void *writeChunk(void *ptr){
    struct args *args = ptr;
    chunk ch;
    while (1) {
        if (checkLoop(&args->iterationsWrite, args->chunks, args->mutexIterWr) == -1) break;
        ch = q_remove(args->out);
        add_chunk(args->ar,ch);
        free_chunk(ch);
    }
    return NULL;
}

void compress(struct args *args, int num_threads, pthread_t *threads, void *(func)(void *)){
    printf("Creando %d threads + 2 (rw).\n",num_threads);
    pthread_create(&threads[0],NULL, readChunk,args);
    for(int i=2;i < num_threads;i++){
        printf("Creado thread %d\n", i+1);
        pthread_create(&threads[i],NULL, func,args);
    }
    pthread_create(&threads[1],NULL, writeChunk,args);
}

void wait(pthread_t *threads, int num_threads,struct args *args){
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    destruirMutex(args->mutexIterWr);
    destruirMutex(args->mutexIterRd);
    destruirMutex(args->mutexIterWk);
    free(args);
}

void initialize_args(struct args *args,queue in, queue out, archive ar, int fd, int size, int chunks){
    args->mutexIterRd = inicializarMutex();
    args->mutexIterWr = inicializarMutex();
    args->mutexIterWk = inicializarMutex();
    args->chunks = chunks;
    args->iterationsRead = 0;
    args->iterationsWrite = 0;
    args->iterationsWorker = 0;
    args->size = size;
    args->fd = fd;
    args->ar = ar;
    args->in = in;
    args->out = out;
}

// take chunks from queue in, run them through process (compress or decompress), send them to queue out
void *worker(void *ptr) {
    struct args *args = ptr;
    chunk ch, res;
    while(1) {
        if(checkLoop(&args->iterationsWorker,args->chunks,args->mutexIterWk)==-1) break;
        ch = q_remove(args->in);

        res = zcompress(ch);
        free_chunk(ch);

        q_insert(args->out, res);
    }
    return NULL;
}

// Compress file taking chunks of opt.size from the input file,
// inserting them into the in queue, running them using a worker,
// and sending the output from the out queue into the archive file
void comp(struct options opt, struct args *args, pthread_t *threads) {
    int fd, chunks;
    char comp_file[256];
    struct stat st;
    archive ar;
    queue in, out;

    if((fd=open(opt.file, O_RDONLY))==-1) {
        printf("Cannot open %s\n", opt.file);
        exit(0);
    }

    fstat(fd, &st);
    chunks = st.st_size/opt.size+(st.st_size % opt.size ? 1:0);

    if(opt.out_file) {
        strncpy(comp_file,opt.out_file,255);
    } else {
        strncpy(comp_file, opt.file, 255);
        strncat(comp_file, ".ch", 255);
    }

    ar = create_archive_file(comp_file);

    in  = q_create(opt.queue_size);
    out = q_create(opt.queue_size);

    initialize_args(args,in,out,ar,fd,opt.size,chunks);

    compress(args,opt.num_threads,threads,worker);

    wait(threads,opt.num_threads,args);
    close_archive_file(ar);
    close(fd);
    q_destroy(in);
    q_destroy(out);
}


// Decompress file taking chunks of size opt.size from the input file

void decomp(struct options opt) {
    int fd, i;
    char uncomp_file[256];
    archive ar;
    chunk ch, res;

    if((ar=open_archive_file(opt.file))==NULL) {
        printf("Cannot open archive file\n");
        exit(0);
    }

    if(opt.out_file) {
        strncpy(uncomp_file, opt.out_file, 255);
    } else {
        strncpy(uncomp_file, opt.file, strlen(opt.file) -3);
        uncomp_file[strlen(opt.file)-3] = '\0';
    }

    if((fd=open(uncomp_file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))== -1) {
        printf("Cannot create %s: %s\n", uncomp_file, strerror(errno));
        exit(0);
    }

    for(i=0; i<chunks(ar); i++) {
        ch = get_chunk(ar, i);

        res = zdecompress(ch);
        free_chunk(ch);

        lseek(fd, res->offset, SEEK_SET);
        write(fd, res->data, res->size);
        free_chunk(res);
    }

    close_archive_file(ar);
    close(fd);
}

int main(int argc, char *argv[]) {
    struct args *args = malloc(sizeof (struct args));
    struct options opt;
    opt.compress    = COMPRESS;
    opt.num_threads = 10;
    opt.size        = CHUNK_SIZE;
    opt.queue_size  = QUEUE_SIZE;
    opt.out_file    = NULL;
    read_options(argc, argv, &opt);
    pthread_t threads[opt.num_threads+2];
    if(opt.compress == COMPRESS) comp(opt,args,threads);
    else decomp(opt);
}