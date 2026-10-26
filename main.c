#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "des_image.h"

#define WIDTH  1920
#define HEIGHT 1080

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

static inline float toCenterScreen(float value, float zvalue, int centerTo) {
    return ((value / (zvalue + 3.5)) * HEIGHT) + centerTo/2.0f;
}

void drawLineVert(DESImage *framebuffer, ObjVertex *v1, ObjVertex *v2, DESColor *color) {
    drawLine(
            framebuffer,
            toCenterScreen(v1->x, v1->z, WIDTH),
            toCenterScreen(v1->y, v1->z, HEIGHT),
            toCenterScreen(v2->x, v2->z, WIDTH),
            toCenterScreen(v2->y, v2->z, HEIGHT),
            color
    );
}

int main() {
#define bpp    3

    int data_size = WIDTH * HEIGHT * bpp;
    uint8_t data[WIDTH * HEIGHT * bpp] = {0};
    DESImageData imageData = {
        data,
        data_size,
    };

    DESImage framebuffer = {
        imageData,
        RGB,
        WIDTH,
        HEIGHT,
    };

#define MAX_VERTICE_COUNT 5000
    static ObjVertex vertices[MAX_VERTICE_COUNT];

#define MAX_FACE_COUNT 10000
    static ObjFace faces[MAX_FACE_COUNT];

    char *filename = "./obj/diablo3_pose/diablo3_pose.obj";
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file %s\n", filename);
        return 1;
    }

#define MAX_LINE_LEN 255
    char buffer[MAX_LINE_LEN] = {0};
    int curr_vert_index = 0;
    int curr_face_index = 0;
    while (fgets(buffer, MAX_LINE_LEN, file) != NULL) {
        // Parse vertice
        if (strncmp(buffer, "v ", 2) == 0) {
            char *start = &buffer[2];
            char *endptr  = NULL;
            //TODO: verify the parsing is successful
            vertices[curr_vert_index].x = strtof(start, &endptr);
            vertices[curr_vert_index].y = strtof(endptr, &endptr);
            vertices[curr_vert_index].z = strtof(endptr, &endptr);
            curr_vert_index++;
        }
        // Parse face
        if (strncmp(buffer, "f ", 2) == 0) {
            char *start = &buffer[2];
            int index1 = atoi(start);
            while (!isspace(*start++)) {}
            int index2 = atoi(start);
            while (!isspace(*start++)) {}
            int index3 = atoi(start);
            faces[curr_face_index].v1 = &vertices[index1 - 1];
            faces[curr_face_index].v2 = &vertices[index2 - 1];
            faces[curr_face_index].v3 = &vertices[index3 - 1];
            curr_face_index++;
        }
    }

    for (int i = 0; i < curr_face_index; i++) {
        drawLineVert(&framebuffer, faces[i].v1, faces[i].v2, &RED);
        drawLineVert(&framebuffer, faces[i].v2, faces[i].v3, &RED);
        drawLineVert(&framebuffer, faces[i].v3, faces[i].v1, &RED);
    }
    for (int i = 0; i < curr_vert_index; i++) {
        desimage_set(
                &framebuffer,
                toCenterScreen(vertices[i].x, vertices[i].z, WIDTH),
                toCenterScreen(vertices[i].y, vertices[i].z, HEIGHT),
                &WHITE
        );
    }

    //int ax =  7, ay =  3;
    //int bx = 12, by = 37;
    //int cx = 62, cy = 53;

    //drawLine(&framebuffer, ax, ay, bx, by, &BLUE);
    //drawLine(&framebuffer, cx, cy, bx, by, &GREEN);
    //drawLine(&framebuffer, cx, cy, ax, ay, &YELLOW);
    //drawLine(&framebuffer, ax, ay, cx, cy, &RED);

    //desimage_set(&framebuffer, ax, ay, &WHITE);
    //desimage_set(&framebuffer, bx, by, &WHITE);
    //desimage_set(&framebuffer, cx, cy, &WHITE);

    //perf measuring
    //srand(time(NULL));
    //for (int i=0; i<(1<<24); i++) {
    //    int ax = rand()%WIDTH, ay = rand()%HEIGHT;
    //    int bx = rand()%WIDTH, by = rand()%HEIGHT;
    //    drawLine(&framebuffer, ax, ay, bx, by, &(DESColor){{ rand()%255, rand()%255, rand()%255, rand()%255 }});
    //}

    desimage_write_tga_file(&framebuffer, "framebuffer.tga");
    return 0;
}
