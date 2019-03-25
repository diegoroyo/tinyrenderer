#include <stdint.h>
#include <cstring>
#include <fstream>
#include <iostream>

#include "pngchunk.h"

// Corregir el valor leido en el archivo .png (Big Endian)
uint32_t swap_bytes(uint8_t* read) {
    return read[3] | read[2] << 8 | read[1] << 16 | read[0] << 24;
}

uint32_t calculate_crc(uint32_t dividend, uint8_t* stream, int streamLength) {
    // TODO
}

PNGChunk::PNGChunk() : length(0), chunkType(nullptr), data(nullptr), crc(0) {}

bool PNGChunk::read_file(std::ifstream& is) {
    if (!is.good()) {
        std::cerr << "An error ocurred while reading the data\n";
        return false;
    }

    // Longitud (4 bytes)
    uint8_t length_aux[4];
    is.read((char*)length_aux, 4);
    this->length = swap_bytes(length_aux);

    // Tipo (4 bytes)
    this->chunkType = new uint8_t[5]{0};
    is.read((char*)&this->chunkType, 4);

    // Datos
    is.read((char*)this->data, this->length);

    // CRC
    uint8_t crc_aux[4];
    is.read((char*)crc_aux, 4);
    this->crc = swap_bytes(crc_aux);

    // Comprobar CRC correcto
    uint32_t calc_crc =
        calculate_crc(swap_bytes(this->chunkType), this->data, this->length);

    if (calc_crc == this->crc) {
        return true;
    } else {
        std::cerr << "Incorrect chunk CRC\n";
        return false;
    }
}

PNGChunk::~PNGChunk() {
    delete[] data;
    delete[] chunkType;
}