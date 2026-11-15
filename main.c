#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "tga_image.h"
#include <float.h>

#define WIDTH  1080
#define HEIGHT 1080

static TGAColor WHITE   = {{255, 255, 255, 255}}; // attention, BGRA order
static TGAColor GREEN   = {{  0, 255,   0, 255}};
static TGAColor RED     = {{  0,   0, 255, 255}};
static TGAColor BLUE    = {{255, 128,  64, 255}};
static TGAColor YELLOW  = {{  0, 200, 255, 255}};

double Viewport[4][4], Perspective[4][4], ModelView[4][4];

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

void viewport(const int x, const int y, const int w, const int h) {
    const double v[4][4] = {
        {w/2.0, 0,     0,  x+w/2.0},
        {0,     h/2.0, 0,  y+h/2.0},
        {0,     0,     1,  0},
        {0,     0,     0,  1}
    };

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            Viewport[row][col] = v[row][col];
        }
    }
}

void perspective(const double f) {
    double p[4][4] = {
        {1, 0,  0,   0},
        {0, 1,  0,   0},
        {0, 0,  1,   0},
        {0, 0, -1/f, 1}
    };

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            Perspective[row][col] = p[row][col];
        }
    }
}

void lookat(const Vec3 eye, const Vec3 center, const Vec3 up) {
    Vec3 n = normalizedVec3(restVec3(eye, center));
    Vec3 l = normalizedVec3(cross3(up,n));
    Vec3 m = normalizedVec3(cross3(n, l));
    double a[4][4] = {
        {l.x, l.y, l.z, 0},
        {m.x, m.y, m.z, 0},
        {n.x, n.y, n.z, 0},
        {0,   0,   0,   1}
    };

    double b[4][4] = {
        {1, 0, 0, -center.x},
        {0, 1, 0, -center.y},
        {0, 0, 1, -center.z},
        {0, 0, 0,  1       }
    };

    mat4Mult(ModelView, a, b);
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
            model->faces[model->faces_count].v1_index = index1 - 1;
            model->faces[model->faces_count].v2_index = index2 - 1;
            model->faces[model->faces_count].v3_index = index3 - 1;
            model->faces_count++;
        }
    }

    fclose(file);
    return true;
}

void findBoundingBox(const Vec2 *points, const int points_len, Vec2 *out_points) {
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

//double signedTriangleArea(const Vec2 *a, const Vec2 *b, const Vec2 *c) {
//     return 0.5 * ((b->y - a->y)*(b->x + a->x) + (c->y - b->y)*(c->x + b->x) + (a->y - c->y)*(a->x + c->x));
//}

Vec4 mat4MultVec4(double m[4][4], Vec4 v) {
    return (Vec4) {
        m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w,
        m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w,
        m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w,
        m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w
    };
}

Vec3 mat3MultVec3(double m[3][3], Vec3 v) {
    return (Vec3) {
        m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z,
        m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z,
        m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z
    };
}

double matrix3Det(const double m[3][3]) {
    return m[0][0] * (m[1][1]*m[2][2] - m[1][2]*m[2][1])
         - m[0][1] * (m[1][0]*m[2][2] - m[1][2]*m[2][0])
         + m[0][2] * (m[1][0]*m[2][1] - m[1][1]*m[2][0]);
}

int matrix3InvertTranspose(double out[3][3], const double m[3][3]) {
    double c00 =  (m[1][1]*m[2][2] - m[1][2]*m[2][1]);
    double c01 = -(m[1][0]*m[2][2] - m[1][2]*m[2][0]);
    double c02 =  (m[1][0]*m[2][1] - m[1][1]*m[2][0]);

    double c10 = -(m[0][1]*m[2][2] - m[0][2]*m[2][1]);
    double c11 =  (m[0][0]*m[2][2] - m[0][2]*m[2][0]);
    double c12 = -(m[0][0]*m[2][1] - m[0][1]*m[2][0]);

    double c20 =  (m[0][1]*m[1][2] - m[0][2]*m[1][1]);
    double c21 = -(m[0][0]*m[1][2] - m[0][2]*m[1][0]);
    double c22 =  (m[0][0]*m[1][1] - m[0][1]*m[1][0]);

    double det = m[0][0]*c00 + m[0][1]*c01 + m[0][2]*c02;
    if (fabs(det) < 1e-12) return 0;

    double inv = 1.0 / det;
    out[0][0] = c00*inv; out[0][1] = c01*inv; out[0][2] = c02*inv;
    out[1][0] = c10*inv; out[1][1] = c11*inv; out[1][2] = c12*inv;
    out[2][0] = c20*inv; out[2][1] = c21*inv; out[2][2] = c22*inv;
    return 1;
}

void rasterize(TGAImage *framebuffer, double *depthbuffer, Vec4 clip[3], TGAColor *color) {
    Vec4 ndc[3]    = {
        vec4DivDouble(clip[0], clip[0].w),
        vec4DivDouble(clip[1], clip[1].w),
        vec4DivDouble(clip[2], clip[2].w)
    };
    Vec2 screen[3];// = { (Viewport*ndc[0]).xy(), (Viewport*ndc[1]).xy(), (Viewport*ndc[2]).xy() }; // screen coordinates

    Vec4 temp = mat4MultVec4(Viewport, ndc[0]);
    screen[0] = (Vec2) {temp.x, temp.y};
    temp = mat4MultVec4(Viewport, ndc[1]);
    screen[1] = (Vec2) {temp.x, temp.y};
    temp = mat4MultVec4(Viewport, ndc[2]);
    screen[2] = (Vec2) {temp.x, temp.y};

    double ABC[3][3] = {
        {screen[0].x, screen[0].y, 1.0},
        {screen[1].x, screen[1].y, 1.0},
        {screen[2].x, screen[2].y, 1.0}
    };

    if (matrix3Det(ABC) < 1) return; // backface culling + discarding triangles that cover less than a pixel
                                                                                                 //

    Vec2 box_points[2];
    findBoundingBox(screen, 3, box_points);
    int x_start = box_points[0].x < 0 ? 0 : (int)box_points[0].x;
    int x_end   = box_points[1].x > framebuffer->width  - 1 ? framebuffer->width  - 1 : (int)box_points[1].x;
    int y_start = box_points[0].y < 0 ? 0 : (int)box_points[0].y;
    int y_end   = box_points[1].y > framebuffer->height - 1 ? framebuffer->height - 1 : (int)box_points[1].y;


    for (int y = y_start; y <= y_end; y++) {
        for (int x = x_start; x <= x_end; x++) {
            double inverted[3][3];
            matrix3InvertTranspose(inverted, ABC);
            Vec3 bc = mat3MultVec3(inverted, (Vec3){(double)(x), (double)(y), 1.0}); // barycentric coordinates of {x,y} w.r.t the triangle
            if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;                                                    // negative barycentric coordinate => the pixel is outside the triangle
            double z = dot3(bc, (Vec3){ ndc[0].z, ndc[1].z, ndc[2].z });
            if (z > depthbuffer[y * framebuffer->width + x]) {
                depthbuffer[y * framebuffer->width + x] = z;
                tgaimage_set(framebuffer, x, y, color);
            }

            //Vec2 p = {x, y};
            //double alpha = signedTriangleArea(&p, &points[1], &points[2]) / total_area;
            //double beta  = signedTriangleArea(&p, &points[2], &points[0]) / total_area;
            //double gamma = signedTriangleArea(&p, &points[0], &points[1]) / total_area;
            //if (alpha < 0 || beta < 0 || gamma < 0) {
            //    continue;
            //}

            //// Outline
            ////if (alpha > 0.05 && beta > 0.05 && gamma > 0.05) {
            ////    continue;
            ////}

            //uint8_t z = (alpha * clip[0].z + beta * clip[1].z + gamma * clip[2].z);
            ////gradient color
            ////TGAColor color = {{alpha * az, beta * bz, gamma * cz, 0}};

            //if (z > depthbuffer->imageData.data[y * depthbuffer->width + x]) {
            //    tgaimage_set(depthbuffer, x, y, &(TGAColor){{z}});
            //    tgaimage_set(framebuffer, x, y, color);
            //}
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

Vec4 applyTransforms(Vec3 v) {
    double result[4][4];
    mat4Mult(result, Perspective, ModelView);

    return (Vec4){
        result[0][0]*v.x + result[0][1]*v.y + result[0][2]*v.z + result[0][3],
        result[1][0]*v.x + result[1][1]*v.y + result[1][2]*v.z + result[1][3],
        result[2][0]*v.x + result[2][1]*v.y + result[2][2]*v.z + result[2][3],
        result[3][0]*v.x + result[3][1]*v.y + result[3][2]*v.z + result[3][3]
    };

}

void drawObjModel(TGAImage *framebuffer, double *depthbuffer, ObjModel *model) {
    for (int i = 0; i < model->faces_count; i++) {
        Vec4 clip[3];
        clip[0] = applyTransforms(model->vertices[model->faces[i].v1_index]);
        clip[1] = applyTransforms(model->vertices[model->faces[i].v2_index]);
        clip[2] = applyTransforms(model->vertices[model->faces[i].v3_index]);
        TGAColor randColor = getRandColor();
        rasterize(
            framebuffer,
            depthbuffer,
            clip,
            &randColor
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
    perspective(dot3(rest, rest));                        // build the Perspective matrix
    viewport(WIDTH/16, HEIGHT/16, WIDTH*7/8, HEIGHT*7/8); // build the Viewport    matrix

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
    //    drawObjModel(&framebuffer, &depthbuffer, &head, xoffset, 0);
    //}

    //static ObjModel african;
    //char *filename = "./obj/african_head/african_head.obj";
    //if (loadObjModelFromFile(filename, &african)) {
    //    drawObjModel(&framebuffer, &depthbuffer, &african, 0, 0);
    //}

    static ObjModel diablo;
    char *filename = "./obj/diablo3_pose/diablo3_pose.obj";
    if (loadObjModelFromFile(filename, &diablo)) {
        drawObjModel(&framebuffer, depthbuffer, &diablo);
    }

    tgaimage_write_tga_file(&framebuffer, "framebuffer.tga");
    //tgaimage_write_tga_file(&depthbuffer, "depth.tga");
    return 0;
}
