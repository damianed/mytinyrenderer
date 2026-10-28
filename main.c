#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "tga_image.h"

#define WIDTH  128
#define HEIGHT 128

static TGAColor WHITE   = {{255, 255, 255, 255}}; // attention, BGRA order
static TGAColor GREEN   = {{  0, 255,   0, 255}};
static TGAColor RED     = {{  0,   0, 255, 255}};
static TGAColor BLUE    = {{255, 128,  64, 255}};
static TGAColor YELLOW  = {{  0, 200, 255, 255}};

typedef struct {
    int x;
    int y;
} Coords;

void swapInts(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void drawLine(TGAImage *framebuffer, int ax, int ay, int bx, int by, TGAColor *color) {
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
            tgaimage_set(framebuffer, y, x, color);
        } else {
            tgaimage_set(framebuffer, x, y, color);
        }
        ierror += 2 * abs(by-ay);
        y += (by > ay ? 1 : -1) * (ierror > bx - ax);
        ierror -= 2 * (bx-ax)   * (ierror > bx - ax);
    }
}

static inline float toCenterScreen(float value, float zvalue, int centerTo) {
    return ((value / (zvalue + 3.5)) * HEIGHT) + centerTo/2.0f;
}

void drawLineVert(TGAImage *framebuffer, ObjVertex *v1, ObjVertex *v2, TGAColor *color) {
    drawLine(
            framebuffer,
            toCenterScreen(v1->x, v1->z, WIDTH),
            toCenterScreen(v1->y, v1->z, HEIGHT),
            toCenterScreen(v2->x, v2->z, WIDTH),
            toCenterScreen(v2->y, v2->z, HEIGHT),
            color
    );
}

bool loadObjModelFromFile(char *filename, ObjModel *model) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file %s\n", filename);
        return false;
    }

#define MAX_LINE_LEN 255
    char buffer[MAX_LINE_LEN] = {0};
    while (fgets(buffer, MAX_LINE_LEN, file) != NULL) {
        // Parse vertice
        if (strncmp(buffer, "v ", 2) == 0) {
            char *start = &buffer[2];
            char *endptr  = NULL;
            //TODO: verify the parsing is successful
            model->vertices[model->vertex_count].x = strtof(start, &endptr);
            model->vertices[model->vertex_count].y = strtof(endptr, &endptr);
            model->vertices[model->vertex_count].z = strtof(endptr, &endptr);
            model->vertex_count++;
        }
        // Parse face
        if (strncmp(buffer, "f ", 2) == 0) {
            char *start = &buffer[2];
            int index1 = atoi(start);
            while (!isspace(*start++)) {}
            int index2 = atoi(start);
            while (!isspace(*start++)) {}
            int index3 = atoi(start);
            model->faces[model->faces_count].v1_index = index1 - 1;
            model->faces[model->faces_count].v2_index = index2 - 1;
            model->faces[model->faces_count].v3_index = index3 - 1;
            model->faces_count++;
        }
    }

    return true;
}

void drawObjModel(TGAImage *framebuffer, ObjModel *model) {
    for (int i = 0; i < model->faces_count; i++) {
        ObjVertex *v1 = &model->vertices[model->faces[i].v1_index];
        ObjVertex *v2 = &model->vertices[model->faces[i].v2_index];
        ObjVertex *v3 = &model->vertices[model->faces[i].v3_index];

        drawLineVert(framebuffer, v1, v2, &RED);
        drawLineVert(framebuffer, v2, v3, &RED);
        drawLineVert(framebuffer, v3, v1, &RED);
    }

    for (int i = 0; i < model->vertex_count; i++) {
        tgaimage_set(
            framebuffer,
            toCenterScreen(model->vertices[i].x, model->vertices[i].z, WIDTH),
            toCenterScreen(model->vertices[i].y, model->vertices[i].z, HEIGHT),
            &WHITE
        );
    }
}

void drawTriangle(TGAImage *framebuffer, int ax, int ay, int bx, int by, int cx, int cy, TGAColor *color) {
    int a1x = ax;
    int b1x = bx;
    int a1y = ay;
    int b1y = by;

    int a2x = bx;
    int b2x = cx;
    int a2y = by;
    int b2y = cy;
    bool steep  = abs(a1x-b1x) < abs(a1y-b1y);
    bool steep2 = abs(a2x-b2x) < abs(a2y-b2y);
    if (steep) { // if the line is steep, we transpose the image
        swapInts(&a1x, &a1y);
        swapInts(&b1x, &b1y);
    }

    if (a1x > b1x) { // make it left−to−right
        swapInts(&a1x, &b1x);
        swapInts(&a1y, &b1y);
    }

    if (steep2) {
        swapInts(&a2x, &a2y);
        swapInts(&b2x, &b2y);
    }

    if (a2x > b2x) { // make it left−to−right
        swapInts(&a2x, &b2x);
        swapInts(&a2y, &b2y);
    }

    int y = a1y;
    int ierror = 0;

    //TODO: this should be what ever is biggest between W or H
    Coords steps1[WIDTH] = {0};
    int steps1_count = 0;
    for (int x = a1x; x <= b1x; x++) {
        if (steep) { // if transposed, de−transpose
            tgaimage_set(framebuffer, y, x, color);
            steps1[steps1_count] = (Coords) {y, x};
        } else {
            tgaimage_set(framebuffer, x, y, color);
            steps1[steps1_count] = (Coords) {x, y};
        }
        steps1_count++;
        ierror += 2 * abs(b1y-a1y);
        y += (b1y > a1y ? 1 : -1) * (ierror > b1x - a1x);
        ierror -= 2 * (b1x-a1x)   * (ierror > b1x - a1x);
    }

    y = a2y;
    ierror = 0;

    //TODO: this should be what ever is biggest between W or H
    Coords steps2[WIDTH] = {0};
    int steps2_count = 0;
    for (int x = a2x; x <= b2x; x++) {
        if (steep2) { // if transposed, de−transpose
            tgaimage_set(framebuffer, y, x, color);
            steps2[steps2_count] = (Coords) {y, x};
        } else {
            tgaimage_set(framebuffer, x, y, color);
            steps2[steps2_count] = (Coords) {x, y};
        }
        steps2_count++;
        ierror += 2 * abs(b2y-a2y);
        y += (b2y > a2y ? 1 : -1) * (ierror > b2x - a2x);
        ierror -= 2 * (b2x-a2x)   * (ierror > b2x - a2x);
    }

    //TODO: just testing this should be >= 0 but need to think about how to handle it
    while (steps1_count > 0 || steps2_count > 0) {
        if (steps1_count > 0) {
            steps1_count--;
        }
        if (steps2_count > 0) {
            steps2_count--;
        }

        drawLine(framebuffer, steps1[steps1_count].x, steps1[steps1_count].y, steps2[steps2_count].x, steps2[steps2_count].y, color);
    }
}

int main() {
#define bpp    3

    int data_size = WIDTH * HEIGHT * bpp;
    uint8_t data[WIDTH * HEIGHT * bpp] = {0};
    TGAImageData imageData = {
        data,
        data_size,
    };

    TGAImage framebuffer = {
        imageData,
        RGB,
        WIDTH,
        HEIGHT,
    };

    //static ObjModel model;
    //char *filename = "./obj/diablo3_pose/diablo3_pose.obj";
    //if (loadObjModelFromFile(filename, &model)) {
    //    drawObjModel(&framebuffer, &model);
    //}

    drawTriangle(&framebuffer,   7, 45, 35, 100, 45,  60, &RED);
    drawTriangle(&framebuffer, 120, 35, 90,   5, 45, 110, &WHITE);
    drawTriangle(&framebuffer, 115, 83, 80,  90, 85, 120, &GREEN);

    //int ax =  7, ay =  3;
    //int bx = 12, by = 37;
    //int cx = 62, cy = 53;

    //drawLine(&framebuffer, ax, ay, bx, by, &BLUE);
    //drawLine(&framebuffer, cx, cy, bx, by, &GREEN);
    //drawLine(&framebuffer, cx, cy, ax, ay, &YELLOW);
    //drawLine(&framebuffer, ax, ay, cx, cy, &RED);

    //tgaimage_set(&framebuffer, ax, ay, &WHITE);
    //tgaimage_set(&framebuffer, bx, by, &WHITE);
    //tgaimage_set(&framebuffer, cx, cy, &WHITE);

    //perf measuring
    //srand(time(NULL));
    //for (int i=0; i<(1<<24); i++) {
    //    int ax = rand()%WIDTH, ay = rand()%HEIGHT;
    //    int bx = rand()%WIDTH, by = rand()%HEIGHT;
    //    drawLine(&framebuffer, ax, ay, bx, by, &(TGAColor){{ rand()%255, rand()%255, rand()%255, rand()%255 }});
    //}

    tgaimage_write_tga_file(&framebuffer, "framebuffer.tga");
    return 0;
}
