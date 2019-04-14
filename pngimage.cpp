#include <fstream>
#include <iostream>

#include "pngchunk.h"
#include "pngimage.h"

PNGImage::PNGImage() : width(0), height(0) {}

// Datos descomprimidos de un chunk IDAT, añadir a la imagen
// https://stackoverflow.com/questions/49017937/png-decompressed-idat-chunk-how-to-read
bool PNGImage::read_IDAT_info(int& pixelX, int& pixelY,
                              PNGChunk::IDATInfo* info) {
    int i = 0;
    int filterType = 0;
    bool readOk = true;
    while (readOk && i < info->blockLength) {
        // Cada fila comienza con un byte para indicar tipo de filtro
        // Solo se soporta tipos 0, 1 y 2 (None, Sub y Add)
        // https://www.w3.org/TR/PNG-Filters.html
        if (pixelX == 0) {
            if (info->blockData[i] > 2) {
                // TODO filtros 3 y 4
                std::cerr << "Error: unsupported filter type: "
                          << (int)info->blockData[i]
                          << " (supported types are None, Sub and Add)"
                          << std::endl;
                readOk = false;
            } else {
                this->pixels[pixelY] = new PNGImage::RGBColor*[this->width];
                filterType = info->blockData[i];
                i++;  // leido un byte
            }
        }
        // Leer datos del pixel (r, g, b 3 bytes en ese orden)
        if (readOk) {
            uint8_t r = info->blockData[i];
            uint8_t g = info->blockData[i + 1];
            uint8_t b = info->blockData[i + 2];
            if (filterType == 1 && pixelX > 0) {
                // Sub(x) = Raw(x) - Raw(x-bpp)
                r ^= this->pixels[pixelY][pixelX - 1]->r;
                g ^= this->pixels[pixelY][pixelX - 1]->g;
                b ^= this->pixels[pixelY][pixelX - 1]->b;
            } else if (filterType == 2 && pixelY > 0) {
                // Up(x) = Raw(x) - Prior(x)
                r ^= this->pixels[pixelY - 1][pixelX]->r;
                g ^= this->pixels[pixelY - 1][pixelX]->g;
                b ^= this->pixels[pixelY - 1][pixelX]->b;
            }
            this->pixels[pixelY][pixelX] = new PNGImage::RGBColor(r, g, b);
            i = i + 3;  // leidos 3 bytes
            pixelX++;
            if (pixelX == this->width) {
                pixelX = 0;
                pixelY++;
            }
        }
    }
    return readOk;
}

bool PNGImage::read_png_file(const char* filename) {
    std::ifstream is(filename, std::ios::binary);
    if (!is.is_open()) {
        std::cerr << "Can't open file " << filename << std::endl;
        is.close();
        return false;
    }
    // Comparar la cabecera del archivo para verificar que es PNG
    uint8_t header[HEADER_LENGTH];
    is.read((char*)header, HEADER_LENGTH);
    if (!is.good()) {
        std::cerr << "Can't read header for " << filename << std::endl;
        is.close();
        return false;
    } else {
        for (int i = 0; i < HEADER_LENGTH; i++) {
            if (header[i] != HEADER_SIGNATURE[i]) {
                std::cerr << "File " << filename
                          << " is not recognized as a PNG file" << std::endl;
                std::cerr << "Expected header: ";
                for (int j = 0; j < HEADER_LENGTH; j++) {
                    std::cerr << std::hex << int(HEADER_SIGNATURE[j]) << " ";
                }
                std::cerr << std::endl << "Instead got: ";
                for (int j = 0; j < HEADER_LENGTH; j++) {
                    std::cerr << std::hex << int(header[j]) << " ";
                }
                std::cerr << std::endl;
                is.close();
                return false;
            }
        }
    }

    // Leer información de la imagen, chunk a chunk
    bool endChunkRead = false;
    bool headerChunkRead = false;
    bool isImageOk = true;  // lectura ha ido bien
    int pixelX = 0;         // ej: imagen 2x2, el pixel (1, 1) (abajo derecha)
    int pixelY = 0;         // corresponde con pixelX = 1, pixelY = 1
    while (isImageOk && !endChunkRead) {
        PNGChunk chunk;
        if (!is.good() || !chunk.read_file(is)) {
            if (is.eof()) {
                std::cerr << "Warning: finished reading without IEND chunk"
                          << std::endl;
            } else {
                std::cerr << "Read invalid chunk, stopping" << std::endl;
            }
            isImageOk = false;
        } else {
            if (chunk.is_type("IHDR")) {
                PNGChunk::IHDRInfo* info =
                    dynamic_cast<PNGChunk::IHDRInfo*>(chunk.chunkInfo);
                // Solo se da soporte a imagenes PNG sencillas (solo color, sin
                // paletas ni alpha)
                if (info->compression == 0 && info->filter == 0 &&
                    info->interlace == 0 && info->colorType == 2 &&
                    info->bitDepth == 8) {
                    this->width = info->width;
                    this->height = info->height;
                    this->pixels = new PNGImage::RGBColor**[this->height];
                    headerChunkRead = true;
                } else {
                    isImageOk = false;
                }
            } else if (headerChunkRead) {
                if (chunk.is_type("IDAT")) {
                    PNGChunk::IDATInfo* info =
                        dynamic_cast<PNGChunk::IDATInfo*>(chunk.chunkInfo);
                    isImageOk = read_IDAT_info(pixelX, pixelY, info);
                } else if (chunk.is_type("IEND")) {
                    endChunkRead = true;
                } else {
                    std::cerr << "Warning: unrecognized chunk type"
                              << std::endl;
                }
            } else {
                std::cerr << "Error: the first chunk is not an IHDR chunk"
                          << std::endl;
                isImageOk = false;
            }
        }
    }

    // Evitar acceso a los datos de la imagen si no se ha leido correctamente
    if (!isImageOk) {
        this->width = 0;
        this->height = 0;
    }

    return isImageOk;
}

// Devuelve el color del pixel (x, y) de la imagen
// o bien nullptr si se sale de la cuadrícula
PNGImage::RGBColor* PNGImage::get_pixel(int x, int y) {
    if (x >= 0 && x < this->width && y >= 0 && y < this->height) {
        return pixels[y][x];
    } else {
        return nullptr;
    }
}

PNGImage::~PNGImage() {
    for (int h = 0; h < this->height; h++) {
        for (int w = 0; w < this->width; w++) {
            delete this->pixels[h][w];
        }
        delete this->pixels[h];
    }
    delete this->pixels;
}