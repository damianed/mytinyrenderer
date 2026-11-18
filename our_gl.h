#include "math.h"
#include "tga_image.h"

Vec4 applyTransforms(Vec3 v);

void lookat(const Vec3 eye, const Vec3 center, const Vec3 up);

void initPerspective(const double f);

void initViewport(const int x, const int y, const int w, const int h);

void rasterize(TGAImage *framebuffer, double *depthbuffer, Vec4 clip[3], TGAColor *color);
