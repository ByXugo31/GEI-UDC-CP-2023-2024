#include "stdio.h"
#include "queue.h"
#include "pthread.h"
#include "stdlib.h"

void *test(void *ptr){
    queue q = ptr;
    int *a = malloc(sizeof (int));
    q_insert(q,a);
    return NULL;
}

void *test2(void *ptr){
    queue q = ptr;
    q_remove(q);
    return NULL;
}

int main(){
    int size = 200;
    /* i  = nº hilos de inserciones = nº operaciones de inserciones
     * i2 = nº hilos de borrados = nº operaciones de borrados
     * Escoger los hilos con este criterio para un buen funcionamiento:
     * Los hilos de inserciones tienen que ser menores que el tamaño.
     * Los hilos de borrados tienen que ser menores o iguales que los de inserción
     * Si estas condiciones no se cumplen, los threads se van a quedar esperando
     * infinitamente, bien sea porque la cola está llena y se están insertando elementos demás
     * o porque la cola está vacía y se están intentando eliminar menos elementos de los disponibles.
     */
    int i = 2;
    int i2 = 2;
    pthread_t pthread[i],pthread1[i2];
    queue q = q_create(size);
    for(int j=0;j<i;j++){
        pthread_create(&pthread[j],NULL, test, q);
    }
    for(int j=0;j<i2;j++){
        pthread_create(&pthread1[j],NULL, test2, q);
    }
    for(int j=0;j<i;j++){
        pthread_join(pthread[j], NULL);
    }
    for(int j=0;j<i2;j++){
        pthread_join(pthread1[j], NULL);
    }
    printf("Hilos inserciones: %d Hilos boorados: %d Tamaño cola: %d Tamaño final cola: %d\n",
           i,i2,size,q_elements(q));
    q_destroy(q);
    return 0;
}