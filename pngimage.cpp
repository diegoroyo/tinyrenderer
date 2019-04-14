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
    bool headerChunkRead = false;
    bool isImageOk = true; // lectura ha ido bien
    while (!endChunkRead) {
        PNGChunk chunk;
        if (!is.good() || !chunk.read_file(is)){
            if (is.eof()) {
                std::cerr << "Warning: finished reading image without IEND chunk" << std::endl;
            } else {
                std::cerr << "Read invalid chunk: stopping loading of image" << std::endl;
            }
            endChunkRead = true;
            isImageOk = false;
        } else {

            if (chunk.is_type("IHDR")) {
                PNGChunk::IHDRInfo* info = dynamic_cast<PNGChunk::IHDRInfo*>(chunk.chunkInfo);
                // Solo se da soporte a imagenes PNG sencillas (solo color, sin paletas ni alpha)
                if (info->compression == 0 && info->filter == 0 && info->interlace == 0
                    && info->colorType == 2 && info->bitDepth == 8) {
                    this->width = info->width;
                    this->height = info->height;
                    headerChunkRead = true;
                } else {
                    endChunkRead = true;
                    isImageOk = false;
                }
            } else if (headerChunkRead) {
                if (chunk.is_type("IDAT")) {
                    PNGChunk::IDATInfo* info = dynamic_cast<PNGChunk::IDATInfo*>(chunk.chunkInfo);
                    // TODO info->data contiene los datos de la imagen
                    // https://stackoverflow.com/questions/49017937/png-decompressed-idat-chunk-how-to-read
                } else if (chunk.is_type("IEND")) {
                    endChunkRead = true;
                } else {
                    std::cerr << "Warning: unrecognized chunk type" << std::endl;
                }
            } else {
                std::cerr << "Error: the first chunk is not an IHDR chunk" << std::endl;
                endChunkRead = true;
                isImageOk = false;
            }

        }
    }

    return isImageOk;
}