#include <chrono>
#include <ctime> 
#include <thread>
#include <pthread.h>
#include <omp.h>
#include <filesystem>
#include <winbase.h>
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include "mpi.h"
#include "image.hpp"

using namespace std;
#define THREADS_NUMBER 8
#define BUF_SIZE 256

class Args {
    public:
        vector<Pixel *> pixels;
        int width;
        int height;
        int t_num;
        int* rgb;

        Args(vector<Pixel *> pixels, int width, int height, int t_num, int* rgb) {
            this->pixels = pixels;
            this->height = height;
            this->width = width;
            this->t_num = t_num;
            this->rgb = rgb;
        }
};


class BaseCounter {
    public:
        BaseCounter(Image* img) {
            this->img = img;
            this->pixels = this->img->getPixels();
            this->imgFileName = img->getFileName();
        }

        void print()
        {
            cout << "[ ";
            for (int i = 0; i < 3; i++)
            {
                cout << this->rgb[i] << " ";
            }
            cout << "]\n";
        }

        void count() {
            if (!this->img->hasPixels()) {
                cout << "Ошибка при загрузке изображения\n";
                return;
            }

            this->calcTimeDecorator();
        };

        void count(bool withDecorator) {
            if (withDecorator) {
                this->count();
            } else {
                this->countWrap();
            }
            
        };

    protected:
        int rgb[3] {0, 0, 0};
        Image* img;
        vector<Pixel *> pixels;
        string imgFileName;

        void calcTimeDecorator() {
            auto start = std::chrono::system_clock::now();
            this->countWrap();
            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end-start;
            std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n\n";
        }

        virtual void countWrap() {};

        static DWORD __stdcall countParallel(void *args) {
            Args* params = (Args*)args;

            int dw = params->width / THREADS_NUMBER;
            if (params->t_num == THREADS_NUMBER - 1) {
                dw = params->width - (THREADS_NUMBER - 1)*dw;
            }

            int endw = params->width - (params->width / THREADS_NUMBER) * params->t_num;

            for (int i = endw - 1; i >= endw - dw; i--) {
                for (int j = 0; j < params->height; j++) {
                    params->rgb[params->pixels[params->width*j + i]->getDominantColor()]++;
                }
            }

            return 0;
        }
};


class BruteCounter: public BaseCounter {
    public:
        BruteCounter(Image* img) : BaseCounter(img) {};

    private:
        void countWrap() {
            for (Pixel *p : this->pixels)
            {
                this->rgb[p->getDominantColor()]++;
            }

            cout << "Brute RGB:\n";
            this->print();
        }
};


class CreateThreadWithHeapCreateCounter: public BaseCounter {
    public:
        CreateThreadWithHeapCreateCounter(Image* img) : BaseCounter(img) {};

    private:
        void countWrap() {
            DWORD th_id[THREADS_NUMBER];
            HANDLE th_h[THREADS_NUMBER];

            HANDLE heap[THREADS_NUMBER];
            int* rgbs[THREADS_NUMBER];

            for (int i = 0; i < THREADS_NUMBER; i++) {
                heap[i] = HeapCreate(0, 0, 0);
                rgbs[i] = (int*) HeapAlloc(heap[i], HEAP_ZERO_MEMORY, sizeof(int)*3);
                Args* args = new Args(this->pixels, this->img->getWidth(), this->img->getHeight(), i, rgbs[i]);

                th_h[i] = CreateThread(NULL, 0, BaseCounter::countParallel, args, 0, th_id+i);
            }

            WaitForMultipleObjects(THREADS_NUMBER, th_h, true, INFINITE);

            for (auto rgb: rgbs) {
                for (int i = 0; i < 3; i++) {
                    this->rgb[i] += rgb[i];
                }
            }

            cout << "CreateThreadWithHeapCreate RGB:\n";
            this->print();

            return;
        }
};


class ThreadCounter: public BaseCounter {
    public:
        ThreadCounter(Image* img): BaseCounter(img) {};

    private:
        void countWrap() {
            thread* ths[THREADS_NUMBER];

            HANDLE heap = HeapCreate(0, 0, 0);;
            int* rgbs[THREADS_NUMBER];

            for (int i = 0; i < THREADS_NUMBER; i++) {
                rgbs[i] = (int*) HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(int)*3);
                Args* args = new Args(this->pixels, this->img->getWidth(), this->img->getHeight(), i, rgbs[i]);

                ths[i] = new thread(BaseCounter::countParallel, args);
            }

            for (int i = 0; i < THREADS_NUMBER; i++) {
                ths[i]->join();
            }

            for (auto rgb: rgbs) {
                for (int i = 0; i < 3; i++) {
                    this->rgb[i] += rgb[i];
                }
            }

            cout << "std::thread RGB:\n";
            this->print();

            HeapDestroy(heap);

            return;

        }
};


class PThreadCounter: public BaseCounter {
    public:
        PThreadCounter(Image* img): BaseCounter(img) {};

    private:
        void countWrap() {
            pthread_t ths[THREADS_NUMBER];

            HANDLE heap = HeapCreate(0, 0, 0);;
            int* rgbs[THREADS_NUMBER];

            for (int i = 0; i < THREADS_NUMBER; i++) {
                rgbs[i] = (int*) HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(int)*3);
                Args* args = new Args(this->pixels, this->img->getWidth(), this->img->getHeight(), i, rgbs[i]);

                pthread_create(&ths[i], nullptr, PThreadCounter::countParallel, args);
            }

            for (int i = 0; i < THREADS_NUMBER; i++) {
                pthread_join(ths[i], nullptr);
            }

            for (auto rgb: rgbs) {
                for (int i = 0; i < 3; i++) {
                    this->rgb[i] += rgb[i];
                }
            }

            cout << "pthread RGB:\n";
            this->print();

            HeapDestroy(heap);

            return;
        }

        static void* __stdcall countParallel(void *args) {
            Args* params = (Args*)args;

            int dw = params->width / THREADS_NUMBER;
            if (params->t_num == THREADS_NUMBER - 1) {
                dw = params->width - (THREADS_NUMBER - 1)*dw;
            }

            int endw = params->width - (params->width / THREADS_NUMBER) * params->t_num;

            for (int i = endw - 1; i >= endw - dw; i--) {
                for (int j = 0; j < params->height; j++) {
                    params->rgb[params->pixels[params->width*j + i]->getDominantColor()]++;
                }
            }

            return nullptr;
        }
};


class OMPCounter: public BaseCounter {
    public:
        OMPCounter(Image* img): BaseCounter(img) {};

    private:
        void countWrap() {
            omp_set_num_threads(THREADS_NUMBER);
            
            HANDLE heap = HeapCreate(0, 0, 0);;
            int* rgbs[THREADS_NUMBER];
            int i = 0;

            #pragma omp parallel default(none) shared(rgbs, i, heap)
            {
                #pragma omp for
                for (i = 0; i < THREADS_NUMBER; i++) {
                    rgbs[i] = (int*) HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(int)*3);
                    Args* args = new Args(this->pixels, this->img->getWidth(), this->img->getHeight(), i, rgbs[i]);

                    BaseCounter::countParallel(args);
                }
            }

            #pragma omp barrier

            for (auto rgb: rgbs) {
                for (int i = 0; i < 3; i++) {
                    this->rgb[i] += rgb[i];
                }
            }

            cout << "OMP RGB:\n";
            this->print();

            HeapDestroy(heap);

            return;
        }
};


class CreateProcessCounter: public BaseCounter {
    public:
        CreateProcessCounter(Image* img, int argc, char** argv): BaseCounter(img) {
            this->argc = argc;
            this->argv = argv;
        };

    private:
        int argc;
        char** argv; 

        void countWrap() {
            if (this->argc < 3) {
                char cmd[4096];
                STARTUPINFO si[THREADS_NUMBER];
                PROCESS_INFORMATION pi[THREADS_NUMBER];

                for (int i = 0; i < THREADS_NUMBER; i++) {
                    ZeroMemory(&si[i], sizeof(si[i]));
                    si[i].cb = sizeof(si[i]);
                    ZeroMemory(&pi[i], sizeof(pi[i]));
                    
                    sprintf(cmd, "\"%s\" \"%s\" %d", this->argv[0], this->imgFileName.c_str(), i);
                    CreateProcessA(NULL, cmd, NULL, NULL, true, 0, NULL, NULL, &si[i], &pi[i]);
                }

                for (int i = 0; i < THREADS_NUMBER; i++) {
                    WaitForSingleObject(pi[i].hProcess, INFINITE);
                }


                for (int i = 0; i < THREADS_NUMBER; i++) {
                    int rgbs[3] {0, 0, 0};
                    string name = to_string(i).append(".txt");

                    FILE *f = fopen(TEXT(name.c_str()), "rb");
                    fread(rgbs, sizeof(int), 3, f);

                    for (int i = 0; i < 3; i++) {
                        this->rgb[i] += rgbs[i];
                    }

                    fclose(f);
                    filesystem::remove(name.c_str());
                }    

                cout << "CreateProcess:\n";
                this->print();


            } else {
                int rgbs[3] {0,0,0};
                int i = std::stoi(this->argv[2]);

                Args* args = new Args(this->pixels, this->img->getWidth(), this->img->getHeight(), i, &rgbs[0]);
                BaseCounter::countParallel(args);

                string name = string(this->argv[2]).append(".txt");
                FILE *f = fopen(TEXT(name.c_str()), "wb");
                fwrite(rgbs, sizeof(int), 3, f);
                fclose(f);
            }
        }
};


class CreateProcessMapViewOfFileCounter: public BaseCounter {
    public:
        CreateProcessMapViewOfFileCounter(Image* img, int argc, char** argv): BaseCounter(img) {
            this->argc = argc;
            this->argv = argv;
        };

    private:
        int argc;
        char** argv; 

        void countWrap() {
            if (this->argc < 3) {
                char cmd[4096];
                STARTUPINFO si[THREADS_NUMBER];
                PROCESS_INFORMATION pi[THREADS_NUMBER];

                for (int i = 0; i < THREADS_NUMBER; i++) {
                    ZeroMemory(&si[i], sizeof(si[i]));
                    si[i].cb = sizeof(si[i]);
                    ZeroMemory(&pi[i], sizeof(pi[i]));
                    
                    sprintf(cmd, "\"%s\" \"%s\" %d", this->argv[0], this->imgFileName.c_str(), i);
                    CreateProcessA(NULL, cmd, NULL, NULL, true, 0, NULL, NULL, &si[i], &pi[i]);
                }

                for (int i = 0; i < THREADS_NUMBER; i++) {
                    TCHAR name[8];
                    sprintf(name, "Global\\%d", i);
                    HANDLE hMapFile;

                    hMapFile = OpenFileMappingA(
                        FILE_MAP_ALL_ACCESS,
                        FALSE,
                        name
                    );  

                    if (hMapFile == NULL) {
                        i--;
                    }
                }


                for (int i = 0; i < THREADS_NUMBER; i++) {
                    int rgbs[3] {0, 0, 0};
                    
                    TCHAR name[8];
                    sprintf(name, "Global\\%d", i);

                    HANDLE hMapFile;
                    LPCTSTR pBuf;

                    hMapFile = OpenFileMappingA(
                        FILE_MAP_ALL_ACCESS,
                        FALSE,
                        name
                    );  

                    if (hMapFile == NULL) {
                        cout << "Cant open file mapping: " << GetLastError() << "\n";
                    }

                    pBuf = (LPTSTR) MapViewOfFile(
                        hMapFile,
                        FILE_MAP_ALL_ACCESS,
                        0,
                        0,
                        BUF_SIZE
                    );     

                    if (pBuf == NULL) {
                        cout << "Cant map view of file\n";
                        return;
                    }

                    stringstream stream(pBuf);
                    int k = 0;
                    while(stream){
                        stream >> rgbs[k++];
                    }

                    for (int i = 0; i < 3; i++) {
                        this->rgb[i] += rgbs[i];
                    }

                    TerminateProcess(pi[i].hProcess, 0);
                }    

                cout << "CreateProcess with MapViewOfFile:\n";
                this->print();
            } else {
                int rgbs[3] {0,0,0};
                int i = std::stoi(this->argv[2]);

                Args* args = new Args(this->pixels, this->img->getWidth(), this->img->getHeight(), i, &rgbs[0]);
                BaseCounter::countParallel(args); 

                TCHAR name[8];
                sprintf(name, "Global\\%d", i);
                
                TCHAR msg[BUF_SIZE];
                sprintf(msg, "%d %d %d", rgbs[0], rgbs[1], rgbs[2]);

                HANDLE hMapFile;
                LPCTSTR pBuf;

                hMapFile = CreateFileMappingA(
                    INVALID_HANDLE_VALUE,
                    NULL,               
                    PAGE_READWRITE,     
                    0,                 
                    BUF_SIZE,          
                    name
                );  

                if (hMapFile == NULL) {
                    cout << "Cant create file mapping\n";
                }

                pBuf = (LPTSTR) MapViewOfFile(
                    hMapFile,   
                    FILE_MAP_ALL_ACCESS,
                    0,
                    0,
                    BUF_SIZE
                );   

                if (pBuf == NULL) {
                    cout << "Cant map view of file\n";
                }

                CopyMemory((PVOID)pBuf, msg, (_tcslen(msg) * sizeof(TCHAR)));

                std::chrono::milliseconds timespan(30000);
                std::this_thread::sleep_for(timespan);
            }
        }
};


class Mpi2Counter: public BaseCounter {
    public:
        Mpi2Counter(Image* img, int argc, char** argv): BaseCounter(img) {
            this->argc = argc;
            this->argv = argv;
        };

    private:
        int argc;
        char** argv; 

        void countWrap() {
            int myrank, tag = 0;
            MPI_Status status;
            MPI_Init(&this->argc, &this->argv);
            MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

            if (myrank == 0) {
                int rgbs[THREADS_NUMBER][3] {};
                MPI_Request req[THREADS_NUMBER];
                
                for (int i = 0; i < THREADS_NUMBER; i++) {
                    MPI_Irecv(&rgbs[i][0], 3, MPI_INT, i+1, tag, MPI_COMM_WORLD, &req[i]);
                }

                for (int i = 0; i < THREADS_NUMBER; i++) {
                    MPI_Wait(&req[i], &status);
                }

                for (int i = 0; i < THREADS_NUMBER; i++) {
                    for (int k = 0; k < 3; k++) {
                        this->rgb[k] += rgbs[i][k];
                    }
                }

                cout << "MPI2 Async:\n";
                this->print();
            } else {
                int rgbs[3] {0,0,0};
                int i = myrank - 1;

                Args* args = new Args(this->pixels, this->img->getWidth(), this->img->getHeight(), i, &rgbs[0]);
                BaseCounter::countParallel(args); 

                MPI_Request req;
                MPI_Isend(&rgbs[0], 3, MPI_INT, 0, tag, MPI_COMM_WORLD, &req);
            }

            MPI_Finalize();
        }
    
};