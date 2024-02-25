#include <iostream>
#include <vector>
#include <Windows.h>

#include "solvers.hpp"

// A = 1 Потоки с конца 2-мерного массива
// B = 8 Число потоков/процессов

int main(int argc, char** argv)
{
    if (argc < 3) {
        string path = "assets/monkey.jpg";
        
        if (argc == 2) {
            path = argv[1];
        }

        Image *img = new Image(path);

        ThreadCounter* threadCounter = new ThreadCounter(img);
        threadCounter->count();

        OMPCounter* ompCounter = new OMPCounter(img);
        ompCounter->count();

        CreateProcessCounter* createProcessCounter = new CreateProcessCounter(img, argc, argv);
        createProcessCounter->count();

        img->clear();
        return 0;
    }

    string path = argv[1];
    Image *img = new Image(path);

    CreateProcessCounter* createProcessCounter = new CreateProcessCounter(img, argc, argv);
    createProcessCounter->count(false);

    return 0;
}