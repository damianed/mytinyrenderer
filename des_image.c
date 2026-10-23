#include <stdio.h>
#include "des_image.h"
#include <string.h>

void desimage_set(DESImage *framebuffer, int x, int y, DESColor *c) {
    if (x < 0 || y < 0 ||
            x >= framebuffer->width ||
            y >= framebuffer->height
    )
    {
        return;
    }
    memcpy(framebuffer->imageData.data+(x+y*framebuffer->width)*framebuffer->bpp, c->bgra, framebuffer->bpp);
}

bool desimage_unload_rle_data(DESImage *framebuffer, FILE *out) {
    uint8_t max_chunk_length = 128;
    size_t npixels = framebuffer->width*framebuffer->height;
    size_t curpix = 0;
    while (curpix < npixels) {
        size_t chunkstart = curpix*framebuffer->bpp;
        size_t curbyte = curpix*framebuffer->bpp;
        uint8_t run_length = 1;
        bool raw = true;
        uint8_t *data = framebuffer->imageData.data;
        while (curpix+run_length<npixels && run_length<max_chunk_length) {
            bool succ_eq = true;
            for (int t=0; succ_eq && t< (int)framebuffer->bpp; t++)
                succ_eq = (data[curbyte+t]==data[curbyte+t+framebuffer->bpp]);
            curbyte += framebuffer->bpp;
            if (1==run_length)
                raw = !succ_eq;
            if (raw && succ_eq) {
                run_length--;
                break;
            }
            if (!raw && !succ_eq)
                break;
            run_length++;
        }
        curpix += run_length;
        int w = fputc(raw ? run_length - 1 : run_length + 127, out);
        if (w == EOF) return false;
        size_t written = fwrite(data + chunkstart, (raw?run_length*framebuffer->bpp:framebuffer->bpp), 1, out);
        //out.write(reinterpret_cast<const char *>(data.data()+chunkstart), (raw?run_length*framebuffer->bpp:framebuffer->bpp));
        if (written != 1) return false;
    }
    return true;
}


bool desimage_write_tga_file(DESImage *framebuffer, char *path) {
    static uint8_t developer_area_ref[4] = {0, 0, 0, 0};
    static uint8_t extension_area_ref[4] = {0, 0, 0, 0};
    static uint8_t footer[18] = {'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'};
    //std::ofstream out;

    FILE *out = fopen(path, "wb");
    if (!out) {
        fprintf(stderr, "Can't open file %s\n", path);
        return false;
    }
    DESHeader header = {};
    header.bitsperpixel = framebuffer->bpp<<3;
    header.width        = framebuffer->width;
    header.height       = framebuffer->height;
    //header.datatypecode = (framebuffer->bpp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10: 2));
    header.datatypecode = (framebuffer->bpp == GRAYSCALE ? 11 : 10);
    //header.imagedescriptor = vflip ? 0x00 : 0x20; // top-left or bottom-left origin
    header.imagedescriptor = 0x00; // top-left or bottom-left origin

    size_t written = fwrite(&header, sizeof(header), 1, out);
    if (written != 1) goto err;

    //if (!rle) {
    //    out.write(reinterpret_cast<const char *>(data.data()), w*h*bpp);
    //    if (!out.good()) goto err;
    //} else if (!unload_rle_data(out)) goto err;
    if (!desimage_unload_rle_data(framebuffer, out)) goto err;

    written = fwrite(&developer_area_ref, sizeof(developer_area_ref), 1, out);
    if (written != 1) goto err;
    written = fwrite(&extension_area_ref, sizeof(extension_area_ref), 1, out);
    if (written != 1) goto err;
    written = fwrite(&footer, sizeof(footer), 1, out);
    if (written != 1) goto err;

    fclose(out);
    return true;
err:
    fprintf(stderr, "can't dump the tga file\n");
    fclose(out);
    return false;
}
