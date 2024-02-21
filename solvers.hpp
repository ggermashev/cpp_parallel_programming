#include <chrono>
#include <ctime> 
#include "image.hpp"

#define THREADS_NUMBER 8

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

    protected:
        int rgb[3] {0, 0, 0};
        Image* img;
        vector<Pixel *> pixels;

        void calcTimeDecorator() {
            auto start = std::chrono::system_clock::now();
            this->countWrap();
            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end-start;
            std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n\n";
        }

        virtual void countWrap() {};
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

        void countWrap() {
            DWORD th_id[THREADS_NUMBER];
            HANDLE th_h[THREADS_NUMBER];

            HANDLE heap[THREADS_NUMBER];
            int* rgbs[THREADS_NUMBER];

            for (int i = 0; i < THREADS_NUMBER; i++) {
                heap[i] = HeapCreate(0, 0, 0);
                rgbs[i] = (int*) HeapAlloc(heap[i], HEAP_ZERO_MEMORY, sizeof(int)*3);
                Args* args = new Args(this->pixels, this->img->getWidth(), this->img->getHeight(), i, rgbs[i]);

                th_h[i] = CreateThread(NULL, 0, CreateThreadWithHeapCreateCounter::countParallel, args, 0, th_id+i);
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