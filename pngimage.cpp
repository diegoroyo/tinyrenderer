#include <fstream>
#include <iostream>

#include "pngchunk.h"
#include "pngimage.h"

PNGImage::PNGImage() : width(0), height(0) {}

// Modo de filtrado #4, mira los pixeles a la izquierda y derecha
// a: izquierda, b: arriba, c: arriba a la izquierda
uint8_t PNGImage::paeth_pred(uint8_t a, uint8_t b, uint8_t c) {
    uint8_t p = a + b - c;                    // estimación inicial
    uint8_t pa = p - a >= 0 ? p - a : a - p;  // abs(p - a)
    uint8_t pb = p - b >= 0 ? p - b : b - p;  // abs(p - b)
    uint8_t pc = p - c >= 0 ? p - c : c - p;  // abs(p - c)
    // devolver más cercano a a, b, c
    // en caso de empate, a > b > c
    return pa <= pb && pa <= pc ? a : (pb <= pc ? b : c);
}

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
            if (info->blockData[i] > 4) {
                std::cerr << "Error: invalid filter type" << std::endl;
                readOk = false;
            } else {
                this->pixels[pixelY] = new PNGImage::RGBColor*[this->width];
                filterType = info->blockData[i];
                i++;  // leido un byte
            }
        }
        // Leer datos del pixel (r, g, b 3 bytes en ese orden)
        if (readOk) {
            uint16_t r = info->blockData[i], g = info->blockData[i + 1],
                     b = info->blockData[i + 2];
            PNGImage::RGBColor *left, *top, *leftTop;
            if (pixelX > 0) {
                left = this->pixels[pixelY][pixelX - 1];
                if (pixelY > 0) {
                    leftTop = this->pixels[pixelY - 1][pixelX - 1];
                }
            }
            if (pixelY > 0) {
                top = this->pixels[pixelY - 1][pixelX];
            }
            if (filterType == 1 && pixelX > 0) {
                // Sub(x) = Raw(x) - Raw(x-bpp)
                r += left->r;
                g += left->g;
                b += left->b;
            } else if (filterType == 2 && pixelY > 0) {
                // Up(x) = Raw(x) - Prior(x)
                r += top->r;
                g += top->g;
                b += top->b;
            } else if (filterType == 3 && pixelX > 0 && pixelY > 0) {
                // Average(x) = Raw(x) - floor((Raw(x-bpp)+Prior(x))/2)
                r += (left->r + top->r) / 2;
                g += (left->g + top->g) / 2;
                b += (left->b + top->b) / 2;
            } else if (filterType == 4 && pixelX > 0 && pixelY > 0) {
                // Paeth(x) = Raw(x) - PaethPredictor(Raw(x-bpp), Prior(x),
                //   Prior(x-bpp))
                r += paeth_pred(left->r, top->r, leftTop->r);
                g += paeth_pred(left->g, top->g, leftTop->g);
                b += paeth_pred(left->b, top->b, leftTop->b);
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
                if (info->is_supported()) {
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

// Escribe los chunks mínimos para poder ver la imagen
// - Chunk IHDR (24 bit RGB, color, sin paletas)
// - Chunk(s) IDAT (tamaño maximo 8192 bits)
//   TODO
bool PNGImage::write_png_file(const char* filename) {
    std::ofstream os(filename, std::ios::binary);
    if (!os.is_open()) {
        std::cerr << "Can't open file " << filename << std::endl;
        os.close();
        return false;
    }
    // Header archivo PNG
    os.write((char*)HEADER_SIGNATURE, HEADER_LENGTH);
    // Chunk IHDR
    PNGChunk::IHDRInfo infoIHDR(width, height);
    infoIHDR.write_file(os);
    // Chunk IDAT
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

// Si las coordenadas (x, y) pertenecen a la foto (no se salen),
// modifica el color de dicho pixel
void PNGImage::set_pixel(int x, int y, PNGImage::RGBColor* color) {
    if (x >= 0 && x < this->width && y >= 0 && y < this->height) {
        pixels[y][x] = color;
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