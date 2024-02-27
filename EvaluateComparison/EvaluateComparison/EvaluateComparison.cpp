#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <iomanip>

int threads_created = 0; // Variable global para contar la cantidad de hilos creados

// Función para particionar el arreglo en base a un pivote
int partition(std::vector<int>& arr, int low, int high) {
    int pivot = arr[high];
    int i = low - 1;

    for (int j = low; j <= high - 1; j++) {
        if (arr[j] < pivot) {
            i++;
            std::swap(arr[i], arr[j]);
        }
    }

    std::swap(arr[i + 1], arr[high]);
    return i + 1;
}

// Función para realizar el ordenamiento Quick Sort de forma secuencial
void quicksort(std::vector<int>& arr, int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);

        quicksort(arr, low, pi - 1);
        quicksort(arr, pi + 1, high);
    }
}

// Función para realizar el ordenamiento Quick Sort de forma paralela
void quicksort_parallel(std::vector<int>& arr, int low, int high, int depth) {
    if (low < high) {
        if (depth <= 0) {
            quicksort(arr, low, high);
        }
        else {
            int pi = partition(arr, low, high);

            // Crear hilos para ordenar recursivamente cada mitad del arreglo
            std::thread left_thread(quicksort_parallel, std::ref(arr), low, pi - 1, depth - 1);
            std::thread right_thread(quicksort_parallel, std::ref(arr), pi + 1, high, depth - 1);

            // Aumentar el contador de hilos creados
            threads_created += 2;

            // Esperar a que los hilos terminen su ejecución
            left_thread.join();
            right_thread.join();
        }
    }
}

int main() {
    std::srand(std::time(nullptr));

    // Generar un arreglo aleatorio de tamaño variable
    int arr_size = 1000000;
    std::vector<int> arr(arr_size);
    for (int i = 0; i < arr_size; ++i) {
        arr[i] = std::rand() % 1000;
    }

    // Clonar el arreglo para usar en la versión paralela
    std::vector<int> arr_parallel = arr;
    int depth = 2;
    int count_processors = 4;

    // Medir el tiempo de ejecución del Quick Sort secuencial
    auto start_time = std::chrono::steady_clock::now();
    quicksort(arr, 0, arr_size - 1);
    auto end_time = std::chrono::steady_clock::now();
    auto sequential_time = end_time - start_time;
    std::cout << "Tiempo de ejecucion de Quick Sort secuencial: " << std::chrono::duration_cast<std::chrono::milliseconds>(sequential_time).count() << " ms\n";

    // Medir el tiempo de ejecución del Quick Sort paralelo
    start_time = std::chrono::steady_clock::now();
    threads_created = 0; // Reiniciar el contador de hilos creados
    quicksort_parallel(arr_parallel, 0, arr_size - 1, depth);
    end_time = std::chrono::steady_clock::now();
    auto parallel_time = end_time - start_time;
    std::cout << "Tiempo de ejecucion de Quick Sort paralelo: " << std::chrono::duration_cast<std::chrono::milliseconds>(parallel_time).count() << " ms\n";
    std::cout << "Cantidad de hilos creados: " << threads_created << std::endl;

    double speedup = static_cast<double>(sequential_time.count()) / parallel_time.count();
    double efficiency = speedup / static_cast<double>(count_processors);

    std::cout << std::fixed << std::setprecision(5);
    std::cout << "Speedup: " << speedup << std::endl;
    std::cout << "Efficency: " << efficiency * 100 << "%" << std::endl;

    return 0;
}
