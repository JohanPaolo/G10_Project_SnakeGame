#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <conio.h>
#include <windows.h>

using namespace std;

// Dimensiones del área de juego
const int width = 40;
const int height = 20;


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

            // Esperar a que los hilos terminen su ejecución
            left_thread.join();
            right_thread.join();
        }
    }
}

void setConsoleWindowSize(int width, int height) {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console == INVALID_HANDLE_VALUE) {
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
    if (!GetConsoleScreenBufferInfo(console, &bufferInfo)) {
        return;
    }

    SMALL_RECT& winInfo = bufferInfo.srWindow;
    COORD largestConsole = GetLargestConsoleWindowSize(console);
    if (width > largestConsole.X) {
        width = largestConsole.X;
    }
    if (height > largestConsole.Y) {
        height = largestConsole.Y;
    }

    COORD size = { width, height };
    SetConsoleScreenBufferSize(console, size);

    SMALL_RECT info = { 0, 0, width - 1, height - 1 };
    SetConsoleWindowInfo(console, TRUE, &info);
}

class Semaphore {
public:
    Semaphore(int count = 1) : count(count) {}

    void notify() {
        unique_lock<mutex> lock(mutex_);
        count++;
        cv.notify_one();
    }

    void wait() {
        unique_lock<mutex> lock(mutex_);
        while (count == 0) {
            cv.wait(lock);
        }
        count--;
    }

private:
    mutex mutex_;
    condition_variable cv;
    int count;
};

Semaphore gameSemaphore(1);

class SnakeGame {
public:
    // Constructor:
    SnakeGame() : gameOver(false), score(0), direction('d'), gameSpeed(100) {
        snake.push_back(make_pair(0, 0));  // Inicializa la serpiente en la esquina sup izq
        spawnFood(); // Función que genera la comida
    }

    ~SnakeGame() {
        if (inputThread.joinable())
            inputThread.join();

        if (gameThread.joinable())
            gameThread.join();

        // Desprender el hilo de Olivia después de que los hilos del juego hayan terminado
        detachAuxiliar();

        // Mostrar recursos utilizados por cada hilo
        cout << "Recursos utilizados por el hilo de entrada: " << inputThreadResources << endl;
        cout << "Recursos utilizados por el hilo del juego: " << gameThreadResources << endl;
    }

    void runGame() {
        // Iniciar 2 hilos
        inputThread = thread(&SnakeGame::processInput, this);
        gameThread = thread(&SnakeGame::updateGame, this);

        // Esperar a que los hilos terminen
        if (inputThread.joinable())
            inputThread.join();

        if (gameThread.joinable())
            gameThread.join();
    }

private:
    vector<pair<int, int>> snake;  // Almacena las coordenadas de cada parte del cuerpo de la serpiente.
    pair<int, int> food;  // Posición de la comida en el tablero.
    bool gameOver;  // Booleano que indica si el juego ha terminado.
    int score;  // Almacena la puntuación del jugador.
    char direction;  // Indica la dirección en la que se está moviendo la serpiente.
    bool paused = false;
    int gameSpeed;  // Velocidad del juego en milisegundos
    int velocidad = 100;
    int foodCount = 0;  // Contador de comida
    unsigned int elapsedTime = 0;  // Variable para almacenar el tiempo transcurrido

    thread auxiliarThread{ process_executer };
    thread inputThread;
    thread gameThread;

    static void process_executer() {
        ofstream logFile("cleaner.txt", ios::app);  // Abre el archivo en modo de anexar

        while (true) {
            logFile << "Proceso siendo ejecutado" << endl;
            this_thread::sleep_for(chrono::milliseconds(2000));
        }

        logFile.close();  // Cierra el archivo al finalizar
    }

    void detachAuxiliar() {
        if (auxiliarThread.joinable()) {
            auxiliarThread.detach();
        }
    }

    unsigned int inputThreadResources = 0;
    unsigned int gameThreadResources = 0;

    void processInput() {
        while (!gameOver) {
            char key = _getch(); // Lee una tecla presionada sin necesidad de presionar "Enter"
            gameSemaphore.wait();  // Espera a que el semáforo esté disponible
            switch (key) {
            case 'w':
                if (direction != 's')  // Evita invertir la dirección y colisionar consigo misma
                    direction = 'w';  // Establece la dirección hacia arriba
                break;
            case 'a':
                if (direction != 'd')  // Evita invertir la dirección y colisionar consigo misma
                    direction = 'a';  // Establece la dirección hacia la izquierda
                break;
            case 's':
                if (direction != 'w')  // Evita invertir la dirección y colisionar consigo misma
                    direction = 's';  // Establece la dirección hacia abajo
                break;
            case 'd':
                if (direction != 'a')  // Evita invertir la dirección y colisionar consigo misma
                    direction = 'd';  // Establece la dirección hacia la derecha
                break;
            case 'p':
                // Pausa o reanuda el juego
                paused = !paused;
                break;
            case 'q':
                gameOver = true;  // Establece el fin de juego
                break;
            case '+':
                // Aumentar la velocidad del juego
                if (gameSpeed > 50)
                    gameSpeed -= 10;
                velocidad += 10;
                break;
            case '-':
                // Disminuir la velocidad del juego
                if (gameSpeed < 200)
                    gameSpeed += 10;
                velocidad -= 10;
                break;
            }
            gameSemaphore.notify();  // Libera el semáforo

            inputThreadResources++;
        }
    }

    void updateGame() {
        while (!gameOver) {
            gameSemaphore.wait();  // Espera a que el semáforo esté disponible
            auto start_time = chrono::high_resolution_clock::now();// Registro del tiempo de inicio

            if (!paused) {
                // Mueve la serpiente
                moveSnake();

                // Comprueba colisiones
                checkCollisions();

                // Genera power-ups
                generatePowerUps();

                // Activa power-ups
                activatePowerUp();

                // Dibuja el juego
                draw();
            }

            gameSemaphore.notify();  // Libera el semáforo
            gameThreadResources++;

            auto end_time = chrono::high_resolution_clock::now();  // Registro del tiempo de finalización
            elapsedTime = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();  // Calcula el tiempo transcurrido


            /* Controla la velocidad del juego
               Se utiliza para ralentizar el bucle del juego, controlando la velocidad a la
               que se actualiza el estado del juego */
            this_thread::sleep_for(chrono::milliseconds(gameSpeed));
        }
    }

    void moveSnake() {
        // Define la cabeza de la serpiente
        pair<int, int> newHead = snake[0];

        // Define las teclas que permiten el movimiento de la serpiente
        switch (direction) {
        case 'w':
            newHead.second--;
            break;
        case 'a':
            newHead.first--;
            break;
        case 's':
            newHead.second++;
            break;
        case 'd':
            newHead.first++;
            break;
        }

        // Inserta la nueva cabeza en la primera posición (ilusión de movimiento)
        snake.insert(snake.begin(), newHead);

        // Elimina la última parte del cuerpo si no ha comido comida
        if (newHead != food)
            snake.pop_back();

        // Comprueba si ha comido comida
        if (newHead == food) {
            score += 10;
            foodCount++;
            spawnFood();
        }
    }

    void checkCollisions() {
        // Comprueba si hay choque con las paredes
        if (snake[0].first < 0 || snake[0].first >= width ||
            snake[0].second < 0 || snake[0].second >= height)
            gameOver = true;

        // Comprueba si hay choque consigo misma
        for (int i = 1; i < snake.size(); ++i) {
            if (snake[0] == snake[i])
                gameOver = true;
        }

        if (gameOver) {
            // Guarda la puntuación en un archivo
            saveScore();
        }
    }

    void generatePowerUps() {
        // Genera un power-up aleatorio con una probabilidad del 5%
        srand(static_cast<unsigned>(time(0)));  // Inicializar la semilla del generador de números aleatorios.
        int random = rand() % 100;
        if (random < 5) {
            // Genera una posición aleatoria para el power-up
            powerUp.first = rand() % width;
            powerUp.second = rand() % height;
        }
    }

    void activatePowerUp() {
        // Si la serpiente ha comido el power-up, duplica su tamaño
        if (snake[0] == powerUp) {
            snake.insert(snake.end(), snake.begin(), snake.end());
            powerUp = make_pair(-1, -1); // Elimina el power-up del tablero
            score = score * 2 + 10;
        }
    }

    void spawnFood() {
        // Espera a que ambos hilos alcancen la barrera

        srand(static_cast<unsigned>(time(0)));  // Inicializar la semilla del generador de números aleatorios.
        food.first = rand() % width;  // Se asigna la posición horizontal de la comida.
        food.second = rand() % height;  // Se asigna la posición vertical de la comida.

    }

    void setCursorPosition(int x, int y) {
        // Obtener el handle de la consola actual
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        // Crear una estructura COORD con las coordenadas deseadas
        COORD coord = { (SHORT)x, (SHORT)y };
        // Establecer la posición del cursor en la consola
        SetConsoleCursorPosition(hConsole, coord);
    }

    void draw() {
        system("cls");  // Limpia la consola (funciona en Windows)

        cout << "\033[2J\033[1;1H" << endl;
        setCursorPosition(2, 0);
        // Dibujar cada pared del tablero de manera independiente
        drawWall(0, 0, width + 2, 1); // Dibujar la pared superior
        drawWall(0, height + 2, width + 2, 1); // Dibujar la pared inferior
        drawWall(0, 0, 1, height + 2); // Dibujar la pared izquierda
        drawWall(width + 2, 1, 1, height + 2); // Dibujar la pared derecha

        // Dibujar el resto del juego
        setCursorPosition(1, 1);
        drawGameContent();

        // Dibujar información adicional
        setCursorPosition(1, height + 2);
        drawAdditionalInfo();
    }

    void drawWall(int x, int y, int w, int h) {
        cout << "\033[1;34m" << endl;
        // Dibujar una pared en la posición (x, y) con ancho w y alto h

        // Mover el cursor a la posición inicial
        cout << "\033[" << y << ";" << x << "H";

        // Dibujar la pared
        for (int i = 0; i < h; ++i) {
            for (int j = 0; j < w; ++j) {
                cout << "#";
            }
            cout << endl;
            cout << "\033[" << y + i + 1 << ";" << x << "H"; // Mover el cursor a la siguiente línea
        }
    }

    void drawGameContent() {
        // Dibujar el contenido del juego (serpiente, comida, etc.)

        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                if (j == food.first && i == food.second) {
                    cout << "\033[" << i + 2 << ";" << j + 2 << "H"; // Mover el cursor a la posición de la comida
                    cout << "\033[1;31m*";  // Representa la comida en rojo
                    cout << "\033[0m";  // Restaura el color predeterminado
                }
                else if (j == snake[0].first && i == snake[0].second) {
                    cout << "\033[" << i + 2 << ";" << j + 2 << "H"; // Mover el cursor a la posición de la cabeza de la serpiente
                    cout << "\033[1;32m0";  // Representa la cabeza de la serpiente en verde
                    cout << "\033[0m";  // Restaura el color predeterminado
                }
                else {
                    bool isBodyPart = false;
                    for (int k = 1; k < snake.size(); ++k) {
                        if (i == snake[k].second && j == snake[k].first) {
                            cout << "\033[" << i + 2 << ";" << j + 2 << "H"; // Mover el cursor a la posición de una parte del cuerpo de la serpiente
                            cout << "\033[1;32mo";  // Representa una parte del cuerpo de la serpiente en verde
                            cout << "\033[0m";  // Restaura el color predeterminado
                            isBodyPart = true;
                            break;
                        }
                    }
                    if (!isBodyPart) {
                        if (powerUp.first == j && powerUp.second == i) {
                            cout << "\033[" << i + 2 << ";" << j + 2 << "H"; // Mover el cursor a la posición del power-up
                            cout << "\033[1;35m@"; // Representa el power-up en magenta
                            cout << "\033[0m";  // Restaura el color predeterminado
                        }
                    }
                }
            }
        }
    }

    void drawAdditionalInfo() {
        // Mostrar información adicional como puntuación y velocidad
        cout << "\033[1;37m" << endl;
        cout << "\n > Score: " << score << endl;
        cout << " > Velocidad de actualizacion: " << gameSpeed << " ms" << endl;
        cout << " > Velocidad de juego: " << velocidad << " ms" << endl;
        // Muestra la frecuencia de actualización en la consola
        cout << "Frecuencia de actualizacion: " << elapsedTime << " ms" << endl;

    }

    void saveScore() {
        ofstream scoreFile("scores.txt", ios::app); // Abre el archivo en modo de anexar
        if (scoreFile.is_open()) {
            scoreFile << score << endl;
            scoreFile.close();
            cout << "Puntuación guardada." << endl;

            // Llamar al ordenamiento paralelo después de guardar la puntuación
            cout << "Ordenando puntuaciones..." << endl;
            vector<int> scores; // Vector para almacenar las puntuaciones
            ifstream scoreFileRead("scores.txt"); // Abrir el archivo en modo lectura
            int score;
            while (scoreFileRead >> score) {
                scores.push_back(score);
            }
            scoreFileRead.close();

            // Ordenar las puntuaciones de mayor a menor de forma paralela
            int depth = 2;
            quicksort_parallel(scores, 0, scores.size() - 1, depth);

            // Reabrir el archivo en modo de escritura y sobrescribir con las puntuaciones ordenadas de mayor a menor
            ofstream sortedScoreFile("scores.txt");
            if (sortedScoreFile.is_open()) {
                for (int score : scores) {
                    sortedScoreFile << score << endl;
                }
                sortedScoreFile.close();
            }
            else {
                cerr << "Error al abrir el archivo de puntuaciones para escritura." << endl;
            }
        }
        else {
            cout << "Error al guardar la puntuación." << endl;
        }
    }

    pair<int, int> powerUp;  // Posición del power-up en el tablero
};

int main() {
    // Establecer el tamaño de la consola
    setConsoleWindowSize(40, 30);
    // Creación del objeto SnakeGame
    SnakeGame snakeGame;
    // Correr el juego
    snakeGame.runGame();

    return 0;
}