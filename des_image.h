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
} DESColor;

typedef struct {
    float x;
    float y;
    float z;
} ObjVertex;

typedef struct {
    ObjVertex *v1;
    ObjVertex *v2;
    ObjVertex *v3;
} ObjFace;

typedef struct {
    uint8_t *data;
    int      size;
} DESImageData;

typedef struct {
    DESImageData    imageData;
    BytesPerPixel   bpp;
    int             width;
    int             height;
} DESImage;

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
} DESHeader;
#pragma pack(pop)

void desimage_set(DESImage *framebuffer, int x, int y, DESColor *color);

bool desimage_write_tga_file(DESImage *framebuffer, char *image_path);
