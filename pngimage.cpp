#include <fstream>
#include <iostream>

#include "pngchunk.h"
#include "pngimage.h"

PNGImage::PNGImage() : width(0), height(0) {}

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
    bool isImageOk = true; // lectura ha ido bien
    while (!endChunkRead) {
        PNGChunk chunk;
        if (!chunk.read_file(is)){
            if (is.eof()) {
                std::cerr << "Warning: finished reading image without IEND chunk" << std::endl;
            } else {
                std::cerr << "Read invalid chunk: stopping loading of image" << std::endl;
            }
            endChunkRead = true;
            isImageOk = false;
        } else {

            if (chunk.is_type("IEND")) {
                endChunkRead = true;
            } else if (chunk.is_type("IHDR")) {
                // TODO
            } else if (chunk.is_type("IDAT")) {
                // TODO
            }

        }
    }

    return isImageOk;
}