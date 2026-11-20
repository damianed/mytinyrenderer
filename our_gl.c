#include "our_gl.h"

double Viewport[4][4], Perspective[4][4], ModelView[4][4];

Vec4 mat4MultVec4(double m[4][4], Vec4 v) {
    return (Vec4) {
        m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w,
        m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w,
        m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w,
        m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w
    };
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

void initPerspective(const double f) {
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

void initViewport(const int x, const int y, const int w, const int h) {
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

void rasterize(
        TGAImage *framebuffer,
        double    *depthbuffer,
        GL_Shader *shader,
        bool     (*fragment) (GL_Shader *, const Vec3, TGAColor *color_out),
        Triangle   clip
) {
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

    TGAColor color = {0};
    for (int y = y_start; y <= y_end; y++) {
        for (int x = x_start; x <= x_end; x++) {
            double inverted[3][3];
            matrix3InvertTranspose(inverted, ABC);
            Vec3 bc = mat3MultVec3(inverted, (Vec3){(double)(x), (double)(y), 1.0}); // barycentric coordinates of {x,y} w.r.t the triangle
            if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;                                                    // negative barycentric coordinate => the pixel is outside the triangle
            double z = dot3(bc, (Vec3){ ndc[0].z, ndc[1].z, ndc[2].z });
            if (z > depthbuffer[y * framebuffer->width + x]) {
                bool discard = fragment(shader, bc, &color);
                if (discard) {
                    continue;
                }
                depthbuffer[y * framebuffer->width + x] = z;
                tgaimage_set(framebuffer, x, y, &color);
            }
        }
    }
}
