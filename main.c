#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include "tga_image.h"
#include "our_gl.h"

#define WIDTH  1080
#define HEIGHT 1080

extern double Perspective[4][4], ModelView[4][4];

static TGAColor WHITE   = {{255, 255, 255, 255}}; // attention, BGRA order
static TGAColor GREEN   = {{  0, 255,   0, 255}};
static TGAColor RED     = {{  0,   0, 255, 255}};
static TGAColor BLUE    = {{255, 128,  64, 255}};
static TGAColor YELLOW  = {{  0, 200, 255, 255}};

Vec4 vertex(GL_Shader *shader, const int face, const int vert) {
    Vec3    *vertices = shader->model->vertices;
    ObjFace *faces = shader->model->faces;

    Vec3 v = vertices[faces[face].vert_indexes[vert]];

    Vec4 gl_position = mat4MultVec4(ModelView, (Vec4) {v.x, v.y, v.z, 1});

    shader->tri[vert] = (Vec3) {gl_position.x, gl_position.y, gl_position.z};

    return mat4MultVec4(Perspective, gl_position);
}

bool fragment(GL_Shader *shader, const Vec3 bar, TGAColor *color_out) {
    *color_out = shader->color;
    return false;
}

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

static inline float toCenterScreen(float value) {
    //return ((value / (zvalue + 3.5)) * HEIGHT) + centerTo/2.0f;
    return ((1.0 + value) * HEIGHT)/2.0;
}

void drawLineVert(TGAImage *framebuffer, Vec3 *v1, Vec3 *v2, TGAColor *color) {
    drawLine(
            framebuffer,
            toCenterScreen(v1->x),
            toCenterScreen(v1->y),
            toCenterScreen(v2->x),
            toCenterScreen(v2->y),
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
            model->faces[model->faces_count].vert_indexes[0] = index1 - 1;
            model->faces[model->faces_count].vert_indexes[1] = index2 - 1;
            model->faces[model->faces_count].vert_indexes[2] = index3 - 1;
            model->faces_count++;
        }
    }

    fclose(file);
    return true;
}

//double signedTriangleArea(const Vec2 *a, const Vec2 *b, const Vec2 *c) {
//     return 0.5 * ((b->y - a->y)*(b->x + a->x) + (c->y - b->y)*(c->x + b->x) + (a->y - c->y)*(a->x + c->x));
//}

static inline TGAColor getRandColor() {
    TGAColor rand_color;
    for (int i = 0; i < 3; i++) {
        rand_color.bgra[i] = rand() % 255;
    }

    return rand_color;
}

void drawObjModel(TGAImage *framebuffer, double *depthbuffer, ObjModel *model) {

    GL_Shader shader = {.model = model};
    for (int i = 0; i < model->faces_count; i++) {
        shader.color = getRandColor();
        Triangle clip = {
            vertex(&shader, i, 0),
            vertex(&shader, i, 1),
            vertex(&shader, i, 2),
        };

        rasterize(
            framebuffer,
            depthbuffer,
            &shader,
            fragment,
            clip
        );
    }
}


int main() {
#define bpp    3

    Vec3    eye = {-1, 0, 2}; // camera position
    Vec3 center = {0,  0, 0};  // camera direction
    Vec3     up = {0,  1, 0};  // camera up vector

    lookat(eye, center, up);                              // build the ModelView   matrix
    Vec3 rest = restVec3(eye,center);
    initPerspective(dot3(rest, rest));                        // build the Perspective matrix
    initViewport(WIDTH/16, HEIGHT/16, WIDTH*7/8, HEIGHT*7/8); // build the Viewport    matrix

    int data_size = WIDTH * HEIGHT * bpp;
    static uint8_t data[WIDTH * HEIGHT * bpp] = {0};
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
    static double depthbuffer[WIDTH * HEIGHT];
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        depthbuffer[i] = -DBL_MAX;
    }

    //int xoffset = 400;
    //static ObjModel model;
    //char *filename = "./obj/boggie/body.obj";
    //if (loadObjModelFromFile(filename, &model)) {
    //    drawObjModel(&framebuffer, &depthbuffer, &model, xoffset, 0);
    //}

    //static ObjModel head;
    //filename = "./obj/boggie/head.obj";
    //if (loadObjModelFromFile(filename, &head)) {
    //    drawObjModel(&framebuffer, depthbuffer, &head);
    //}

    static ObjModel african;
    char *filename = "./obj/african_head/african_head.obj";
    if (loadObjModelFromFile(filename, &african)) {
        drawObjModel(&framebuffer, depthbuffer, &african);
    }

   // static ObjModel diablo;
   // char *filename = "./obj/diablo3_pose/diablo3_pose.obj";
   // if (loadObjModelFromFile(filename, &diablo)) {
   //     drawObjModel(&framebuffer, depthbuffer, &diablo);
   // }

    tgaimage_write_tga_file(&framebuffer, "framebuffer.tga");
    //tgaimage_write_tga_file(&depthbuffer, "depth.tga");
    return 0;
}
