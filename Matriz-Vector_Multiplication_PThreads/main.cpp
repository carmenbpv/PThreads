#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <chrono>
#include <iomanip> 

/*
    Compilar: g++ -std=c++14 main.cpp -o main -lpthread
    Ejecutar: ./main <thread_count>
*/

// Numero de threads
int thread_count;

// Numero de filas y columnas de la matriz
int m, n;

// Matriz, vector y vector resultante
double * A;
double * x;
double * y;


// Funcion para realizar la multiplicacion
void * Pth_mat_vect(void * rank) {
    long my_rank = (long)rank;
    int local_m = m / thread_count;
    int my_first_row = my_rank * local_m;
    int my_last_row = (my_rank + 1) * local_m - 1;

    for (int i = my_first_row; i <= my_last_row; i++) {
        y[i] = 0.0;
        for (int j = 0; j < n; j++) {
            y[i] += A[i * n + j] * x[j];
        }
    }
    return NULL;
}

int main (int argc, char * argv[]) {
    long thread;
    pthread_t* thread_handles;

    thread_count = strtol(argv[1], NULL, 10);

    thread_handles = (pthread_t*)malloc(thread_count * sizeof(pthread_t));

    std::cout << "Ingrese el numero de filas: " << std::endl;
    std::cin >> m;

    std::cout << "Ingrese el numero de columnas: " << std::endl;
    std::cin >> n;

    A = (double*)malloc(m * n * sizeof(double));
    x = (double*)malloc(n * sizeof(double));
    y = (double*)malloc(m * sizeof(double));


    /* 
        Matriz A
    */

    // std::cout << "Ingrese la matriz : " << std::endl;

    srand(time(0));
    for (int i = 0; i < m * n; i++) {
        A[i] = (double)(rand() % 10);
        // std::cin >> A[i];
    }

    // Imprimiendo la matriz
    /*
    for (int i = 0; i < m * n; i++) {
        if (i % n == 0 && i > 0) {
            std::cout << std::endl;
        }
        std::cout << A[i] << " ";
    }
    std::cout << std::endl;
    */

    /* 
        Vector x
    */

    // std::cout << "Ingrese el vector : " << std::endl;

    srand(time(0));
    for (int i = 0; i < n; i++) {
        x[i] = (double)(rand() % 10);
        // std::cin >> x[i];
    }

    // Imprimiendo el vector
    /*
    for (int i = 0; i < n; i++) {
        std::cout << x[i] << " ";
    }
    std::cout << std::endl;
    */

    auto start = std::chrono::high_resolution_clock::now();

    for (thread = 0; thread < thread_count; thread++) {
        pthread_create(&thread_handles[thread], NULL, Pth_mat_vect, (void*)thread);
    }

    for (thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], NULL);
    }

    auto end = std::chrono::high_resolution_clock::now();

    double time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  
    time_taken *= 1e-9;    

    std::cout << "El tiempo es: " << std::fixed << time_taken << std::setprecision(9);
    std::cout << " sec." << std::endl;


    // Imprimiendo el resultado
    /*
    std::cout << "Resultado : " << std::endl;
    for (int i = 0; i < m; i++) {
        std::cout << y[i] << " ";
    }
    std::cout << std::endl;
    */

    free(thread_handles);
    free(A);
    free(x);
    free(y);
    return 0;
}

