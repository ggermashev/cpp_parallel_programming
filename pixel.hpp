#include <string>
using namespace std;

#define RED 0
#define GREEN 1
#define BLUE 2
#define UNKNOWN 3

class Pixel {
    public:
        Pixel(int r, int g, int b) {
            this->r = r;
            this->g = g;
            this->b = b;
        }

        int getDominantColor() {
            if (this->r >= this->g && this->r >= this->b) {
                return RED;
            }

            if (this->g >= this->r && this->g >= this->b) {
                return GREEN;
            }

            if (this->b >= this->r && this->b >= this->g) {
                return BLUE;
            }

            return UNKNOWN;
        }

        string toString() {
            char str[16];
            sprintf(str, "%d %d %d", this->r, this->g, this->b);
            return string(str);
        }

    private:
        int r, g, b;
};