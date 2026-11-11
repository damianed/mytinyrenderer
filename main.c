#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "tga_image.h"

#define WIDTH  1920
#define HEIGHT 1080

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

void findBoundingBox(const Coords *points, const int points_len, Coords *out_points) {
    int min_y = points[0].y;
    int max_y = points[0].y;
    int min_x = points[0].x;
    int max_x = points[0].x;
    for (int i = 1; i < points_len; i++) {
        if (points[i].y < min_y) {
            min_y = points[i].y;
        }
        if (points[i].y > max_y) {
            max_y = points[i].y;
        }

        if (points[i].x < min_x) {
            min_x = points[i].x;
        }
        if (points[i].x > max_x) {
            max_x = points[i].x;
        }
    }

    out_points[0].y = min_y;
    out_points[0].x = min_x;
    out_points[1].y = max_y;
    out_points[1].x = max_x;
}

double signedTriangleArea(const Coords *a, const Coords *b, const Coords *c) {
     return 0.5 * ((b->y - a->y)*(b->x + a->x) + (c->y - b->y)*(c->x + b->x) + (a->y - c->y)*(a->x + c->x));
}

void drawTriangle(TGAImage *framebuffer, TGAImage *depthbuffer, int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, TGAColor *color) {
#define POINTS_LEN 3
    Coords points[POINTS_LEN] = {
        {ax, ay},
        {bx, by},
        {cx, cy},
    };

    double total_area = signedTriangleArea(&points[0], &points[1], &points[2]);
    if (total_area < 1) return; // backface culling + discarding triangles that cover less than a pixel

    Coords box_points[2];
    findBoundingBox(points, POINTS_LEN, box_points);

    for (int y = box_points[0].y; y <= box_points[1].y; y++) {
        for (int x = box_points[0].x; x <= box_points[1].x; x++) {
            Coords p = {x, y};
            double alpha = signedTriangleArea(&p, &points[1], &points[2]) / total_area;
            double beta  = signedTriangleArea(&p, &points[2], &points[0]) / total_area;
            double gamma = signedTriangleArea(&p, &points[0], &points[1]) / total_area;
            if (alpha < 0 || beta < 0 || gamma < 0) {
                continue;
            }

            // Outline
            //if (alpha > 0.05 && beta > 0.05 && gamma > 0.05) {
            //    continue;
            //}

            uint8_t z = (alpha * az + beta * bz + gamma * cz);
            //gradient color
            //TGAColor color = {{alpha * az, beta * bz, gamma * cz, 0}};

            if (z > depthbuffer->imageData.data[y * depthbuffer->width + x]) {
                tgaimage_set(depthbuffer, x, y, &(TGAColor){{z}});
                tgaimage_set(framebuffer, x, y, color);
            }
        }
    }
}

static inline TGAColor getRandColor() {
    TGAColor rand_color;
    for (int i = 0; i < 3; i++) {
        rand_color.bgra[i] = rand() % 255;
    }

    return rand_color;
}

void drawObjModel(TGAImage *framebuffer, TGAImage *depthbuffer, ObjModel *model, int xoffset, int yoffset) {
    for (int i = 0; i < model->faces_count; i++) {
        ObjVertex *v1 = &model->vertices[model->faces[i].v1_index];
        ObjVertex *v2 = &model->vertices[model->faces[i].v2_index];
        ObjVertex *v3 = &model->vertices[model->faces[i].v3_index];

        TGAColor randColor = getRandColor();
        drawTriangle(
            framebuffer,
            depthbuffer,
            toCenterScreen(v1->x, v1->z, WIDTH) + xoffset,
            toCenterScreen(v1->y, v1->z, HEIGHT) + yoffset,
            (v1->z + 1.0) * 255.0/2.0,
            toCenterScreen(v2->x, v2->z, WIDTH) + xoffset,
            toCenterScreen(v2->y, v2->z, HEIGHT) + yoffset,
            (v2->z + 1.0) * 255.0/2.0,
            toCenterScreen(v3->x, v3->z, WIDTH) + xoffset,
            toCenterScreen(v3->y, v3->z, HEIGHT) + yoffset,
            (v3->z + 1.0) * 255.0/2.0,
            &randColor
        );

        //drawLineVert(framebuffer, v1, v2, &RED);
        //drawLineVert(framebuffer, v2, v3, &RED);
        //drawLineVert(framebuffer, v3, v1, &RED);
    }

   // for (int i = 0; i < model->vertex_count; i++) {
   //     tgaimage_set(
   //         framebuffer,
   //         toCenterScreen(model->vertices[i].x, model->vertices[i].z, WIDTH),
   //         toCenterScreen(model->vertices[i].y, model->vertices[i].z, HEIGHT),
   //         &WHITE
   //     );
   // }
}


int main() {
#define bpp    3

    int data_size = WIDTH * HEIGHT * bpp;
    uint8_t data[WIDTH * HEIGHT * bpp] = {0};
    TGAImageData imageData = {
        data,
        data_size,
    };

    int depth_size = WIDTH * HEIGHT;
    uint8_t depth_data[WIDTH * HEIGHT] = {0};
    TGAImageData depthimage = {
        depth_data,
        depth_size,
    };

    TGAImage framebuffer = {
        imageData,
        RGB,
        WIDTH,
        HEIGHT,
    };
    TGAImage depthbuffer = {
        depthimage,
        GRAYSCALE,
        WIDTH,
        HEIGHT
    };

    int xoffset = 400;
    static ObjModel model;
    char *filename = "./obj/boggie/body.obj";
    if (loadObjModelFromFile(filename, &model)) {
        drawObjModel(&framebuffer, &depthbuffer, &model, xoffset, 0);
    }

    static ObjModel head;
    filename = "./obj/boggie/head.obj";
    if (loadObjModelFromFile(filename, &head)) {
        drawObjModel(&framebuffer, &depthbuffer, &head, xoffset, 0);
    }

    static ObjModel african;
    filename = "./obj/african_head/african_head.obj";
    if (loadObjModelFromFile(filename, &african)) {
        drawObjModel(&framebuffer, &depthbuffer, &african, 0, 0);
    }

    static ObjModel diablo;
    filename = "./obj/diablo3_pose/diablo3_pose.obj";
    if (loadObjModelFromFile(filename, &diablo)) {
        drawObjModel(&framebuffer, &depthbuffer, &diablo, -xoffset, 0);
    }

    //int ax = 17, ay =  4, az = 255;
    //int bx = 555, by = 399, bz = 255;
    //int cx = 233, cy = 596, cz = 255;
    //drawTriangle(&framebuffer, ax, ay, az, bx, by, bz, cx, cy, cz);
    //drawTriangle2(&framebuffer, 120, 35, 90,   5, 45, 110, &WHITE);
    //drawTriangle2(&framebuffer, 115, 83, 80,  90, 85, 120, &GREEN);

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
    tgaimage_write_tga_file(&depthbuffer, "depth.tga");
    return 0;
}
