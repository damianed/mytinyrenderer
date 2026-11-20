#include "math.h"
#include "tga_image.h"

typedef struct {
    ObjModel *model;
    TGAColor color;
    Vec3     tri[3]; //triangle in eye cordinates
} GL_Shader;

Vec4 applyTransforms(Vec3 v);

Vec4 mat4MultVec4(double m[4][4], Vec4 v);

void lookat(const Vec3 eye, const Vec3 center, const Vec3 up);

void initPerspective(const double f);

void initViewport(const int x, const int y, const int w, const int h);

typedef Vec4 Triangle[3]; // a triangle primitive is made of three ordered points
void rasterize(
        TGAImage  *framebuffer,
        double    *depthbuffer,
        GL_Shader *shader,
        bool     (*fragment) (GL_Shader *, const Vec3, TGAColor *color_out),
        Triangle   clip
);
