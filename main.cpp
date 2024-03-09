#include <iostream>
#include <vector>
#include <Windows.h>

#include "solvers.hpp"

// A = 1 Потоки с конца 2-мерного массива
// B = 8 Число потоков/процессов

int main(int argc, char** argv)
{
    string path = "path/to/img";

    if (argc > 1) {
        path = string(argv[1]);
    }

    Image *img = new Image(path);

    CreateProcessCounter* createProcessCounter = new CreateProcessCounter(img, argc, argv);
    if (argc < 3) {
        createProcessCounter->count();
    } else {
        createProcessCounter->count(false);
    }
    
    img->clear();
    return 0;

}