#include <stdint.h>
#include <cstring>

#pragma once

// Métodos de apoyo para la lectura/escritura de imágenes PNG y su modificación
// Referencia: https://en.wikipedia.org/wiki/Portable_Network_Graphics
class PNGImage {
   private:
    // Todos los archivos PNG tienen comienzan con estos 8 bytes (ver
    // referencia)
    const static int HEADER_LENGTH = 8;
    const uint8_t HEADER_SIGNATURE[HEADER_LENGTH] = {0x89, 0x50, 0x4E, 0x47,
                                                     0x0D, 0x0A, 0x1A, 0x0A};

    int width;
    int height;

   public:
    PNGImage();
    //~PNGImage();
    bool read_png_file(const char* filename);
};