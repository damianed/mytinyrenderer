#include <stdint.h>
#include <stdbool.h>

typedef enum {
    GRAYSCALE = 1,
    RGB       = 3,
    RGBA      = 4,
} BytesPerPixel;

typedef struct {
    uint8_t bgra[4];
    //uint8_t bytespp;
} TGAColor;

typedef struct {
    float x;
    float y;
    float z;
} ObjVertex;

typedef struct {
    int v1_index;
    int v2_index;
    int v3_index;
} ObjFace;

#define MAX_VERTICE_COUNT 5000
#define MAX_FACE_COUNT    10000
typedef struct {
    ObjVertex vertices[MAX_VERTICE_COUNT];
    ObjFace   faces[MAX_FACE_COUNT];
    int       vertex_count;
    int       faces_count;
} ObjModel;

typedef struct {
    uint8_t *data;
    int      size;
} TGAImageData;

typedef struct {
    TGAImageData    imageData;
    BytesPerPixel   bpp;
    int             width;
    int             height;
} TGAImage;

#pragma pack(push, 1)
typedef struct {
    int8_t   idlength;
    uint8_t  colormaptype;
    uint8_t  datatypecode;
    uint16_t colormaporigin;
    uint16_t colormaplength;
    uint8_t  colormapdepth;
    uint16_t x_origin;
    uint16_t y_origin;
    uint16_t width;
    uint16_t height;
    uint8_t  bitsperpixel;
    uint8_t  imagedescriptor;
} TGAHeader;
#pragma pack(pop)

void tgaimage_set(TGAImage *framebuffer, int x, int y, TGAColor *color);

bool tgaimage_write_tga_file(TGAImage *framebuffer, char *image_path);
