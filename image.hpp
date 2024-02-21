#include <Windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "pixel.hpp"

using namespace std;

class Image 
{
    public:
        Image(string fileName)
        {
            this->writeRootPath();
            this->parseImage(fileName);
            this->rawPixelsToRgb();
        }

        vector<Pixel*> getPixels() {
            return this->pixels;
        }

        bool hasPixels() {
            return this->pixels.size() != 0;
        }

        int getHeight() {
            return this->height;
        }

        int getWidth() {
            return this->width;
        }

        void clear() {
            stbi_image_free(this->img);
        }

    private:
        string ROOT_PATH {""};
        unsigned char* img;
        int width, height, channels;
        vector<Pixel*> pixels;

        void writeRootPath()
        {
            char path[256];
            size_t len = sizeof(path);

            int bytes = GetModuleFileName(NULL, path, len);

            if (!bytes) {
                return;
            }

            vector<int> slashes;
            for (int i = 0; i < size(path); i++)
            {
                if (path[i] == '\\')
                {
                    slashes.push_back(i);
                }
            }

            int delimiter = slashes.at(slashes.size() - 1);
            for (int i = 0; i <= delimiter; i++)
            {
                ROOT_PATH += path[i];
            }

            return;
        }

        void parseImage(string fileName) {
            this->img = stbi_load((this->ROOT_PATH + fileName).c_str(), &this->width, &this->height, &this->channels, 0);
            if (this->img == NULL)
            {
                std::cout << "Не удалось загрузить изображение\n";
            }
        }

        void rawPixelsToRgb() {
            if (this->img == NULL) {
                return;
            }

            int size = this->width * this->height * this->channels;
            for (unsigned char *px = this->img; px != this->img + size; px += this->channels) {
                Pixel* p = new Pixel((int)*px, (int)*(px+1), (int)*(px+2));
                this->pixels.push_back(p);
            }
        }
};