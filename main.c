#include "des_image.h"

static DESColor WHITE   = {{255, 255, 255, 255}}; // attention, BGRA order
//static DESColor GREEN   = {  0, 255,   0, 255};
//static DESColor RED     = {  0,   0, 255, 255};
//static DESColor BLUE    = {255, 128,  64, 255};
//static DESColor YELLOW  = {  0, 200, 255, 255};

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

    desimage_set(&framebuffer, ax, ay, &WHITE);
    desimage_set(&framebuffer, bx, by, &WHITE);
    desimage_set(&framebuffer, cx, cy, &WHITE);
    //framebuffer.set(ax, ay, white);
    //framebuffer.set(bx, by, white);
    //framebuffer.set(cx, cy, white);

    desimage_write_tga_file(&framebuffer, "framebuffer.tga");
    return 0;
}
