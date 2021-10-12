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


const int IN_LIST = 1;
const int EMPTY_LIST = -1;
const int END_OF_LIST = 0;


struct list_node_s {
    int data;
    struct list_node_s * next;
    pthread_mutex_t mutex;
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

pthread_mutex_t head_mutex;
pthread_mutex_t count_mutex;

int member_count = 0;
int insert_count = 0;
int delete_count = 0;

void Init_ptrs(struct list_node_s** curr_pp, struct list_node_s** pred_pp) {
    *pred_pp = NULL;
    pthread_mutex_lock(&head_mutex);
    *curr_pp = head;
    if (head != NULL) {
        pthread_mutex_lock(&(head->mutex));
    }
}


int Advance_ptrs(struct list_node_s** curr_pp, struct list_node_s** pred_pp) {
    int rv = IN_LIST;
    struct list_node_s* curr_p = *curr_pp;
    struct list_node_s* pred_p = *pred_pp;

    if (curr_p == NULL){
        if (pred_p == NULL) {
            return EMPTY_LIST;
        }
        else {
            return END_OF_LIST;
        }
    } else {
        if (curr_p->next != NULL) {
            pthread_mutex_lock(&(curr_p->next->mutex));
        }
        else {
            rv = END_OF_LIST;
        }
        if (pred_p != NULL) {
            pthread_mutex_unlock(&(pred_p->mutex));
        }
        else {
            pthread_mutex_unlock(&head_mutex);
        }
        *pred_pp = curr_p;
        *curr_pp = curr_p->next;
        return rv;
    }
}



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
    struct list_node_s* curr;
    struct list_node_s* pred;
    struct list_node_s* temp;
    int rv = 1;

    Init_ptrs(&curr, &pred);
   
    while (curr != NULL && curr->data < value) {
        Advance_ptrs(&curr, &pred);
    }

    if (curr == NULL || curr->data > value) {
        temp = (struct list_node_s *)malloc(sizeof(struct list_node_s));
        pthread_mutex_init(&(temp->mutex), NULL);
        temp->data = value;
        temp->next = curr;
        if (curr != NULL) {
            pthread_mutex_unlock(&(curr->mutex));
        }
        if (pred == NULL) {
            head = temp;
            pthread_mutex_unlock(&head_mutex);
        } else {
            pred->next = temp;
            pthread_mutex_unlock(&(pred->mutex));
        }
    } else {
        if (curr != NULL) {
            pthread_mutex_unlock(&(curr->mutex));
        }
        if (pred != NULL) {
            pthread_mutex_unlock(&(pred->mutex));
        }
        else {
            pthread_mutex_unlock(&head_mutex);
        }
        rv = 0;
    }

    return rv;
}

int Member(int value) {
    struct list_node_s* temp;

    pthread_mutex_lock(&head_mutex);
    temp = head;
    while (temp != NULL && temp->data < value) {
        if (temp->next != NULL) {
            pthread_mutex_lock(&(temp->next->mutex));
        }
        if (temp == head) {
            pthread_mutex_unlock(&head_mutex);
        }
        pthread_mutex_unlock(&(temp->mutex));
        temp = temp->next;
    }

    if (temp == NULL || temp->data > value) {
        if (temp == head) {
            pthread_mutex_unlock(&head_mutex);
        }
        if (temp != NULL) {
            pthread_mutex_unlock(&(temp->mutex));
        }
        return 0;
    } else {
        if (temp == head){
            pthread_mutex_unlock(&head_mutex);
        }
        pthread_mutex_unlock(&(temp->mutex));
        return 1;
    }
}

// Retorna 1 si borro, 0 si el valor no estaba en la lista
int Delete(int value) {
    struct list_node_s* curr;
    struct list_node_s* pred;
    int rv = 1;

    Init_ptrs(&curr, &pred);

    while (curr != NULL && curr->data < value) {
        Advance_ptrs(&curr, &pred);
    }
   
    if (curr != NULL && curr->data == value) {
        if (pred == NULL) {
            head = curr->next;
            pthread_mutex_unlock(&head_mutex);
            pthread_mutex_unlock(&(curr->mutex));
            pthread_mutex_destroy(&(curr->mutex));
            free(curr);
        } else { 
            pred->next = curr->next;
            pthread_mutex_unlock(&(pred->mutex));
            pthread_mutex_unlock(&(curr->mutex));
            pthread_mutex_destroy(&(curr->mutex));
            free(curr);
        }
    } else {
        if (pred != NULL) {
            pthread_mutex_unlock(&(pred->mutex));
        }
        if (curr != NULL) {
            pthread_mutex_unlock(&(curr->mutex));
        }
        if (curr == head) {
            pthread_mutex_unlock(&head_mutex);
        }
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
    int my_member=0, my_insert = 0, my_delete = 0;
    int ops_per_thread = total_operations / thread_count;

    for (i = 0; i < ops_per_thread; i++) {
        which_op = my_drand(&seed);
        val = my_rand(&seed) % MAX_KEY;
        if (which_op < search_percent) {
            Member(val);
            my_member++;
        } else if (which_op < search_percent + insert_percent) {
            Insert(val);
            my_insert++;
        } else {
            Delete(val);
            my_delete++;
        }
    }

    pthread_mutex_lock(&count_mutex);
    member_count += my_member;
    insert_count += my_insert;
    delete_count += my_delete;
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
    pthread_mutex_destroy(&head_mutex);
    pthread_mutex_destroy(&count_mutex);

    return 0;
}

