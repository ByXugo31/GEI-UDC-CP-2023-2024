#include <stdlib.h>

// circular array
typedef struct _queue {
    int size;
    int used;
    int first;
    void **data;
    pthread_mutex_t *mutex;
    pthread_cond_t *cola_llena,*cola_vacia;
} _queue;

#include "queue.h"

void initMutex(queue q){
    pthread_mutex_t *mtx = malloc(sizeof (pthread_mutex_t));
    q->mutex = mtx;
    pthread_mutex_init(q->mutex,NULL);
}

pthread_cond_t *initCond(){
    pthread_cond_t *condicion = malloc(sizeof (pthread_cond_t));
    pthread_cond_init(condicion,NULL);
    return condicion;
}

void destroyMutex(queue q){
    pthread_mutex_destroy(q->mutex);
    free(q->mutex);
}

void destroyCond(pthread_cond_t *condicion){
    pthread_cond_destroy(condicion);
    free(condicion);
}

queue q_create(int size) {
    queue q = malloc(sizeof(_queue));
    initMutex(q);
    q->cola_llena = initCond();
    q->cola_vacia = initCond();
    q->size  = size;
    q->used  = 0;
    q->first = 0;
    q->data  = malloc(size*sizeof(void *));

    return q;
}

int q_elements(queue q) {
    return q->used;
}

int q_insert(queue q, void *elem) {
    pthread_mutex_lock(q->mutex);
    while(q->used == q->size) pthread_cond_wait(q->cola_llena,q->mutex);
    q->data[(q->first+q->used) % q->size] = elem;
    q->used++;
    if (q->used==1) pthread_cond_broadcast(q->cola_vacia);
    pthread_mutex_unlock(q->mutex);
    return 1;
}

void *q_remove(queue q) {
    void *res;
    pthread_mutex_lock(q->mutex);
    while (q->used==0) pthread_cond_wait(q->cola_vacia,q->mutex);
    res = q->data[q->first];
    q->first = (q->first+1) % q->size;
    q->used--;
    if(q->used == q->size-1) pthread_cond_broadcast(q->cola_llena);
    pthread_mutex_unlock(q->mutex);
    return res;
}

void q_destroy(queue q) {
    destroyMutex(q);
    destroyCond(q->cola_llena);
    destroyCond(q->cola_vacia);
    free(q->data);
    free(q);
}
