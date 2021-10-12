#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <chrono>
#include <iomanip> 
#include <semaphore.h>
#include <string.h>
#include <queue>
#include "my_rand.h"

/*
    Compilar: g++ -std=c++14 main.cpp my_rand.c -o main -lpthread
    Ejecutar: ./main <thread_count>
*/

const int MAX_KEY = 1000;

struct list_node_s {
    int data;
    struct list_node_s * next;
};

// Cabeza de la lista
struct list_node_s * head = NULL;
// Numero de threads
int thread_count;

// Para tomar tiempo
int total_operations;

// Porcentaje de operaciones para cada funcion
double insert_percent;
double search_percent;
double delete_percent;

pthread_mutex_t mutex;
pthread_mutex_t count_mutex;

int member_count = 0;
int insert_count = 0;
int delete_count = 0;

// Retorna 1 si lista vacia, 0 caso contrario
int Is_empty () {
    if (head == NULL) {
        return 1;
    }
    else {
        return 0;
    }
}

// Retorna 1 si inserto, 0 si el valor ya estaba en la lista
int Insert(int value) {
    struct list_node_s* curr = head;
    struct list_node_s* pred = NULL;
    struct list_node_s* temp;
    int rv = 1;
   
    while (curr != NULL && curr->data < value) {
        pred = curr;
        curr = curr->next;
    }

    if (curr == NULL || curr->data > value) {
        temp = (struct list_node_s*)malloc(sizeof(struct list_node_s));
        temp->data = value;
        temp->next = curr;
        if (pred == NULL) {
            head = temp;
        }
        else {
            pred->next = temp;
        }
    } else { // El valor ya estaba en la lista
        rv = 0;
    }
    return rv;
}

int Member(int value) {
    struct list_node_s* temp;

    temp = head;
    while (temp != NULL && temp->data < value){
        temp = temp->next;
    }

    if (temp == NULL || temp->data > value) {
        return 0;
    } else {
      return 1;
    }
}

// Retorna 1 si borro, 0 si el valor no estaba en la lista
int Delete(int value) {
    struct list_node_s* curr = head;
    struct list_node_s* pred = NULL;
    int rv = 1;

    while (curr != NULL && curr->data < value) {
        pred = curr;
        curr = curr->next;
    }
   
    if (curr != NULL && curr->data == value) {
        if (pred == NULL) {
            head = curr->next;
            free(curr);
        } else { 
            pred->next = curr->next;
            free(curr);
        }
    } else {
      rv = 0;
    }

    return rv;
}

// Imprimir la lista
void Print () {
    struct list_node_s* temp;
    printf("list = ");

    temp = head;
    while (temp != (struct list_node_s*) NULL) {
        printf("%d ", temp->data);
        temp = temp->next;
    }
    printf("\n");
} 

void * Thread_work(void* rank) {
    long my_rank = (long) rank;
    int i, val;
    double which_op;
    unsigned seed = my_rank + 1;
    int my_member_count = 0, my_insert_count = 0, my_delete_count = 0;
    int ops_per_thread = total_operations / thread_count;

    for (i = 0; i < ops_per_thread; i++) {
        which_op = my_drand(&seed);
        val = my_rand(&seed) % MAX_KEY;
        if (which_op < search_percent) {
            pthread_mutex_lock(&mutex);
            Member(val);
            pthread_mutex_unlock(&mutex);
            my_member_count++;
        } else if (which_op < search_percent + insert_percent) {
            pthread_mutex_lock(&mutex);
            Insert(val);
            pthread_mutex_unlock(&mutex);
            my_insert_count++;
        } else {
            pthread_mutex_lock(&mutex);
            Delete(val);
            pthread_mutex_unlock(&mutex);
            my_delete_count++;
      }
    }

    pthread_mutex_lock(&count_mutex);
    member_count += my_member_count;
    insert_count += my_insert_count;
    delete_count += my_delete_count;
    pthread_mutex_unlock(&count_mutex);
    return NULL;
}  

int main (int argc, char * argv[]) {
    long thread;
    pthread_t* thread_handles;

    int number_of_inserts = 0;

    thread_count = strtol(argv[1], NULL, 10);

    std::cout << "Ingrese el numero de elementos a insertar: " << std::endl;
    std::cin >> number_of_inserts;

    std::cout << "Ingrese el numero total de operaciones: " << std::endl;
    std::cin >> total_operations;

    std::cout << "Ingrese el porcentaje de operaciones que seran busquedas: " << std::endl;
    std::cin >> search_percent;

    std::cout << "Ingrese el porcentaje de operaciones que seran inserciones: " << std::endl;
    std::cin >> insert_percent;

    delete_percent = 1.0 - (search_percent + insert_percent);

    int i = 0;
    int attempts = 0;
    int key = 0;
    int success = 0;
    unsigned seed = 1;

    // Insertando elementos iniciales a la lista
    while (i < number_of_inserts && attempts < 2 * number_of_inserts) {
        key = my_rand(&seed) % MAX_KEY;
        success = Insert(key);
        attempts++;
        if (success) {
            i++;
        }
    }

    std::cout << "Fueron insertados inicialmente " << i << " elmentos en la lista vacia" << std::endl;
    // Imprimiendo la lista incial
    // Print();
    std::cout << std::endl;

    thread_handles = (pthread_t *)malloc(thread_count * sizeof(pthread_t));

    pthread_mutex_init(&count_mutex, NULL);
    pthread_mutex_init(&mutex, NULL);
  

    auto start = std::chrono::high_resolution_clock::now();

    for (thread = 0; thread < thread_count; thread++) {
        pthread_create(&thread_handles[thread], NULL, Thread_work, (void*) thread);
    }

    for (thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], NULL);
    }

    auto end = std::chrono::high_resolution_clock::now();

    double time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  
    time_taken *= 1e-9;    

    std::cout << "El tiempo es: " << std::fixed << time_taken << std::setprecision(9);
    std::cout << " sec." << std::endl;

    std::cout << "Total de operaciones: " << total_operations << std::endl;
    std::cout << "Operaciones miembro: " << member_count << std::endl;
    std::cout << "Operaciones insertar: " << insert_count << std::endl;
    std::cout << "Operaciones borrar: " << delete_count << std::endl;

    free(thread_handles);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&count_mutex);

    return 0;
}

