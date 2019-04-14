#include <stdint.h>
#include <cstring>

#include "pngchunk.h"

#pragma once

// Métodos de apoyo para la lectura/escritura de imágenes PNG y su modificación
// Referencia: https://en.wikipedia.org/wiki/Portable_Network_Graphics
// De momento solo soporta imágenes sin paleta de colores ni alpha
// y con 8 bits de profundidad de color
class PNGImage {
   public:
    class RGBColor {
       public:
        uint8_t r, g, b;
        RGBColor() : r(0), g(0), b(0) {}
        RGBColor(uint8_t _r, uint8_t _g, uint8_t _b) : r(_r), g(_g), b(_b) {}
    };

    int width;
    int height;

    PNGImage();
    ~PNGImage();
    bool read_png_file(const char* filename);
    PNGImage::RGBColor* get_pixel(int x, int y);

   private:
    // Todos los archivos PNG tienen comienzan con estos 8 bytes (ver
    // referencia)
    const static int HEADER_LENGTH = 8;
    const uint8_t HEADER_SIGNATURE[HEADER_LENGTH] = {0x89, 0x50, 0x4E, 0x47,
                                                     0x0D, 0x0A, 0x1A, 0x0A};

    // pixels[y][x] = color del pixel (x, y) de la imagen
    PNGImage::RGBColor*** pixels;

    uint8_t paeth_pred(uint8_t a, uint8_t b, uint8_t c);
    bool read_IDAT_info(int& pixelX, int& pixelY, PNGChunk::IDATInfo* info);
};