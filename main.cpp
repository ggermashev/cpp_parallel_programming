#include <iostream>
#include <vector>
#include <Windows.h>

#include "solvers.hpp"

// A = 1 Потоки с конца 2-мерного массива
// B = 8 Число потоков/процессов

int main(int argc, char** argv)
{
    string path = "assets/monkey.jpg";

    if (argc > 1) {
        path = string(argv[1]);
    }

    Image *img = new Image(path);

    CreateProcessMapViewOfFileCounter* createProcessMapViewOfFileCounter = new CreateProcessMapViewOfFileCounter(img, argc, argv);
    if (argc < 3) {
        createProcessMapViewOfFileCounter->count();
    } else {
        createProcessMapViewOfFileCounter->count(false);
    }
    
    
    img->clear();
    return 0;

}