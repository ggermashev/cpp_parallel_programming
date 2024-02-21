#include <iostream>
#include <vector>
#include <Windows.h>

#include "solvers.hpp"

// A = 1 Потоки с конца 2-мерного массива
// B = 8 Число потоков/процессов

int main(int argc, char** argv)
{
    string path = "path/to/img";
    
    if (argc == 2) {
        path = argv[1];
    }

    Image *img = new Image(path);

    BruteCounter* bruteCounter = new BruteCounter(img);
    bruteCounter->count();

    CreateThreadWithHeapCreateCounter* createThreadWithHeapCreateCounter = new CreateThreadWithHeapCreateCounter(img);
    createThreadWithHeapCreateCounter->count();

    img->clear();
    return 0;
}