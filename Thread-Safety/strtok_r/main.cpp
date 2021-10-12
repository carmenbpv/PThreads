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

/*
    Compilar: g++ -std=c++14 main.cpp -o main -lpthread
    Ejecutar: ./main <thread_count>
*/

const int MAX = 1000;


// Numero de threads
int thread_count;

// Array de semaforos
sem_t* sems;

std::queue <std::string> src;

// Funcion para realizar la multiplicacion
void * Tokenize(void * rank) {
    long my_rank = (long) rank;
    int count;
    int next = (my_rank + 1) % thread_count;
    char * fg_rv = NULL;
    char my_line[MAX];
    char * my_string;
    char * saveptr;

    sem_wait(&sems[my_rank]);

    std::string tmp = src.front();
    src.pop();

    bzero(my_line, MAX);

    for (int i = 0; i < tmp.size() ; i++) {
        my_line[i] = tmp[i];
    }

    fg_rv = my_line;

    //fg_rv = fgets(my_line, MAX, stdin);

    sem_post(&sems[next]);

    while (fg_rv != NULL) {
        printf("Thread %ld > my line = %s\n", my_rank, my_line);

        count = 0;
        my_string = strtok_r(my_line, " \t\n", &saveptr);

        while (my_string != NULL ) {
            count++;
            printf("Thread %ld > string %d = %s\n", my_rank, count, my_string);
            my_string = strtok_r(NULL, " \t\n", &saveptr);
        }
    
        sem_wait(&sems[my_rank]);

        if (!src.empty()) {
            std::string tmp = src.front();
            src.pop();

            bzero(my_line, MAX);

            for (int i = 0; i < tmp.size() ; i++) {
                my_line[i] = tmp[i];
            }

            fg_rv = my_line;

            //fg_rv = fgets(my_line, MAX, stdin);
        } else{
            fg_rv = NULL;
        }
        sem_post(&sems[next]);
    }
    return NULL;
}

int main (int argc, char * argv[]) {
    long thread;
    pthread_t* thread_handles;

    thread_count = strtol(argv[1], NULL, 10);

    thread_handles = (pthread_t *)malloc(thread_count * sizeof(pthread_t));

    sems = (sem_t *)malloc(thread_count * sizeof(sem_t));


    src.push("Pease porridge hot.");
    src.push("Pease porridge cold.");
    src.push("Pease porridge in the pot");
    src.push("Nine days old.");


    sem_init(&sems[0], 0, 1);

    for (thread = 1; thread < thread_count; thread++) {
        sem_init(&sems[thread], 0, 0);
    }

    std::cout << "Ingrese el texto: " << std::endl;

    for (thread = 0; thread < thread_count; thread++) {
        pthread_create(&thread_handles[thread], NULL, Tokenize, (void*) thread);
    }

    for (thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], NULL);
    }

    for (thread = 0; thread < thread_count; thread++) {
        sem_destroy(&sems[thread]);
    }

    free(sems); 
    free(thread_handles);

    return 0;
}

