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
#include "image.hpp"

using namespace std;
// #define THREADS_NUMBER 8
#define THREADS_NUMBER 1
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
                    WaitForSingleObject(pi[i].hProcess, INFINITE);
                }


                for (int i = 0; i < THREADS_NUMBER; i++) {
                    int rgbs[3] {0, 0, 0};
                    
                    TCHAR name[16];
                    sprintf(name, "Global\\%d", i);

                    HANDLE hMapFile;
                    LPCTSTR pBuf;

                    hMapFile = OpenFileMappingA(
                        FILE_MAP_ALL_ACCESS,   // read/write access
                        FALSE,                 // do not inherit the name
                        name                 // name of mapping object
                    );  

                    if (hMapFile == NULL) {
                        cout << "Cant open file mapping: " << i << "\n";
                    }    

                    pBuf = (LPTSTR) MapViewOfFile(
                        hMapFile, // handle to map object
                        FILE_MAP_ALL_ACCESS,  // read/write permission
                        0,
                        0,
                        BUF_SIZE
                    );      

                    if (pBuf == NULL) {
                        cout << "Cant map view of file: " << i << "\n";
                    }

                    for (int i = 0; i < 3; i++) {
                        this->rgb[i] += rgbs[i];
                    }
                }    

                cout << "CreateProcess:\n";
                this->print();
            } else {
                int rgbs[3] {0,0,0};
                int i = std::stoi(this->argv[2]);

                Args* args = new Args(this->pixels, this->img->getWidth(), this->img->getHeight(), i, &rgbs[0]);
                BaseCounter::countParallel(args); 

                TCHAR name[16];
                sprintf(name, "Global\\%d", i);
                
                TCHAR msg[BUF_SIZE];
                sprintf(msg, "%d %d %d", rgbs[0], rgbs[1], rgbs[2]);

                HANDLE hMapFile;
                LPCTSTR pBuf;

                SetLastError(0);
                hMapFile = CreateFileMappingA(
                    INVALID_HANDLE_VALUE,    // use paging file
                    NULL,                    // default security
                    PAGE_READWRITE,          // read/write access
                    0,                       // maximum object size (high-order DWORD)
                    BUF_SIZE,              // maximum object size (low-order DWORD)   
                    name             
                );  
                int err = GetLastError();

                if (hMapFile == NULL) {
                    cout << "Cant create file mapping: " << i << " with error: " << err << "\n";
                }  

                pBuf = (LPTSTR) MapViewOfFile(
                    hMapFile,   // handle to map object
                    FILE_MAP_ALL_ACCESS, // read/write permission
                    0,
                    0,
                    BUF_SIZE
                );    

                if (pBuf == NULL) {
                    cout << "Cant map view of file in thread: " << i << "\n";
                } 

                CopyMemory((PVOID)pBuf, msg, (_tcslen(msg) * sizeof(TCHAR)));

                UnmapViewOfFile(pBuf);
                CloseHandle(hMapFile);
            }
        }
};