#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "options.h"

struct nums {
    int size;
    long *increase;
    long *decrease;
    long total;
    long iterations;
    pthread_mutex_t *mutex_increments;
    pthread_mutex_t *mutex_decrements;
    pthread_mutex_t *iter_mtx;
};

struct args {
    int thread_num;		// application defined thread #
    struct nums *nums;	// pointer to the counters (shared with other threads)
};

struct thread_info {
    pthread_t    id;    // id returned by pthread_create()
    struct args args;  // pointer to the arguments
};

void mutexInit(pthread_mutex_t *mutex, int sz){
    int i = 0;
    while(i<sz){
        pthread_mutex_init(&mutex[i],NULL);
        i++;
    }
}

void mutexDestroy(pthread_mutex_t *mutex, int sz){
    int i = 0;
    while(i<sz){
        pthread_mutex_destroy(&mutex[i]);
        i++;
    }
}

void inicializarArray(long *arr,int sz, long relleno){
    int i = 0;
    while(i<sz){
        arr[i] = relleno;
        i++;
    }
}

long sumarArrays(long *arr1, long *arr2,int sz){
    int i = 0;
    long suma = 0;
    while(i<sz){
        suma += arr1[i] + arr2[i];
        i++;
    }
    return suma;
}

void *desplazamiento(void *ptr){
    struct args *args = ptr;
    struct nums *n = args->nums;
    int randnum, randnum2,min,max;

    while (1) {

        pthread_mutex_lock(n->iter_mtx);
        if(n->iterations<=0){
            pthread_mutex_unlock(n->iter_mtx);
            break;
        }
        n->iterations--;
        pthread_mutex_unlock(n->iter_mtx);

        randnum = rand() % n->size;
        randnum2 = rand() % n->size;

        min = (randnum < randnum2) ? randnum : randnum2;
        max = (randnum > randnum2) ? randnum : randnum2;

        pthread_mutex_lock(&n->mutex_increments[min]);
        n->increase[min]++;
        pthread_mutex_unlock(&n->mutex_increments[min]);
        pthread_mutex_lock(&n->mutex_increments[max]);
        n->increase[max]--;
        pthread_mutex_unlock(&n->mutex_increments[max]);

    }

    return NULL;
}

// Threads run on this function
void *decrease_increase(void *ptr) {
    struct args *args = ptr;
    struct nums *n = args->nums;
    int randnum, randnum2;

    while (1) {

        pthread_mutex_lock(n->iter_mtx);
        if(n->iterations<=0){
            pthread_mutex_unlock(n->iter_mtx);
            break;
        }
        n->iterations--;
        pthread_mutex_unlock(n->iter_mtx);

        randnum = rand() % n->size;
        randnum2 = rand() % n->size;

        pthread_mutex_lock(&n->mutex_increments[randnum]);
        pthread_mutex_lock(&n->mutex_decrements[randnum2]);

        n->decrease[randnum2]--;
        n->increase[randnum]++;

        pthread_mutex_unlock(&n->mutex_decrements[randnum2]);
        pthread_mutex_unlock(&n->mutex_increments[randnum]);


    }

    return NULL;
}

// start opt.num_threads threads running on decrease_incresase
struct thread_info *start_threads(struct options opt, struct nums *nums, struct args *args,void *(func)(void *))
{
    int i;
    struct thread_info *threads;

    printf("creating %d threads\n", opt.num_threads);
    threads = malloc(sizeof(struct thread_info) * opt.num_threads);

    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    // Create num_thread threads running decrease_increase
    for (i = 0; i < opt.num_threads; i++) {
        threads[i].args    = args[i];
        threads[i].args.thread_num = i;
        threads[i].args.nums       = nums;

        if (0 != pthread_create(&threads[i].id, NULL, func, &threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }
    return threads;
}

void print_totals(struct nums *nums)
{
    if(sumarArrays(nums->increase,nums->decrease,nums->size)==nums->total*nums->size){
        printf("OK: Suma arrays: %ld Iteraciones iniciales: %ld Tamaño array: %d\n",
               sumarArrays(nums->increase,nums->decrease,nums->size),nums->total,nums->size);
    } else printf("NOT OK: Suma arrays: %ld Iteraciones iniciales: %ld Tamaño array: %d\n",
                  sumarArrays(nums->increase,nums->decrease,nums->size),nums->total,nums->size);
}

// wait for all threads to finish, print totals, and free memory
void wait(struct options opt, struct nums *nums, struct thread_info *thrs, struct thread_info *thrs2){
    int i;
    for (i = 0; i < opt.num_threads; i++) {
        pthread_join(thrs[i].id, NULL);
        pthread_join(thrs2[i].id, NULL);
    }

    print_totals(nums);

    free(thrs);
    free(thrs2);
}

int main (int argc, char **argv)
{
    struct options opt;
    struct nums nums;
    struct thread_info *thrs;
    struct thread_info *thrs2;
    pthread_mutex_t *general;

    srand(time(NULL));

    // Default values for the options
    opt.num_threads  = 4;
    opt.iterations   = 100000;
    opt.size         = 10;

    read_options(argc, argv, &opt);
    struct args args[opt.num_threads];
    general = malloc(sizeof (pthread_mutex_t));
    nums.iter_mtx = general;
    nums.increase = malloc(opt.size * sizeof (long));
    nums.decrease = malloc(opt.size * sizeof (long));
    nums.mutex_increments = malloc(opt.size * sizeof (pthread_mutex_t));
    nums.mutex_decrements = malloc(opt.size * sizeof (pthread_mutex_t));

    mutexInit(nums.mutex_increments,opt.size);
    mutexInit(nums.mutex_decrements,opt.size);
    pthread_mutex_init(nums.iter_mtx,NULL);

    nums.iterations = opt.iterations;
    nums.size = opt.size;
    nums.total = opt.iterations;

    inicializarArray(nums.increase,opt.size,0);
    inicializarArray(nums.decrease,opt.size,nums.total);

    thrs = start_threads(opt, &nums, args, decrease_increase);
    thrs2 = start_threads(opt,&nums, args, desplazamiento);
    wait(opt, &nums, thrs, thrs2);

    mutexDestroy(nums.mutex_increments,opt.size);
    mutexDestroy(nums.mutex_decrements,opt.size);
    pthread_mutex_destroy(nums.iter_mtx);

    free(nums.increase);
    free(nums.decrease);
    free(nums.mutex_increments);
    free(nums.mutex_decrements);
    free(nums.iter_mtx);

    return 0;
}