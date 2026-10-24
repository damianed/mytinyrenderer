#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "des_image.h"

static DESColor WHITE   = {{255, 255, 255, 255}}; // attention, BGRA order
static DESColor GREEN   = {{  0, 255,   0, 255}};
static DESColor RED     = {{  0,   0, 255, 255}};
static DESColor BLUE    = {{255, 128,  64, 255}};
static DESColor YELLOW  = {{  0, 200, 255, 255}};

void swapInts(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void drawLine(DESImage *framebuffer, int ax, int ay, int bx, int by, DESColor *color) {
    bool steep = abs(ax-bx) < abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        swapInts(&ax, &ay);
        swapInts(&bx, &by);
    }

    if (ax > bx) { // make it left−to−right
        swapInts(&ax, &bx);
        swapInts(&ay, &by);
    }

    int y = ay;
    int ierror = 0;
    for (int x = ax; x <= bx; x++) {
        if (steep) { // if transposed, de−transpose
            desimage_set(framebuffer, y, x, color);
        } else {
            desimage_set(framebuffer, x, y, color);
        }
        ierror += 2 * abs(by-ay);
        y += (by > ay ? 1 : -1) * (ierror > bx - ax);
        ierror -= 2 * (bx-ax)   * (ierror > bx - ax);
    }
}

int main() {
#define width  64
#define height 64
#define bpp    3

    int data_size = width * height * bpp;
    uint8_t data[width * height * bpp] = {0};
    DESImageData imageData = {
        data,
        data_size,
    };

    DESImage framebuffer = {
        imageData,
        RGB,
        width,
        height,
    };

    int ax =  7, ay =  3;
    int bx = 12, by = 37;
    int cx = 62, cy = 53;

    drawLine(&framebuffer, ax, ay, bx, by, &BLUE);
    drawLine(&framebuffer, cx, cy, bx, by, &GREEN);
    drawLine(&framebuffer, cx, cy, ax, ay, &YELLOW);
    drawLine(&framebuffer, ax, ay, cx, cy, &RED);

    desimage_set(&framebuffer, ax, ay, &WHITE);
    desimage_set(&framebuffer, bx, by, &WHITE);
    desimage_set(&framebuffer, cx, cy, &WHITE);

    //perf measuring
    //srand(time(NULL));
    //for (int i=0; i<(1<<24); i++) {
    //    int ax = rand()%width, ay = rand()%height;
    //    int bx = rand()%width, by = rand()%height;
    //    drawLine(&framebuffer, ax, ay, bx, by, &(DESColor){{ rand()%255, rand()%255, rand()%255, rand()%255 }});
    //}

    desimage_write_tga_file(&framebuffer, "framebuffer.tga");
    return 0;
}
