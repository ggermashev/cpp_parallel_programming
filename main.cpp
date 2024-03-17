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

    Mpi2Counter* mpi2 = new Mpi2Counter(img, argc, argv);
    mpi2->count(false);
    

    img->clear();
    return 0;

}