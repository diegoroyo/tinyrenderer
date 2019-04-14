#include <stdint.h>
#include <cstring>
#include <fstream>
#include <iostream>

#include "pngchunk.h"

uint32_t PNGChunk::CRC_TABLE[256];

// Corregir el valor leido en el archivo .png (Big Endian)
uint32_t swap_bytes(uint8_t* read) {
    return read[3] | read[2] << 8 | read[1] << 16 | read[0] << 24;
}

// Cálculo del CRC de 32 bits de un chunk de la imágen PNG
// Para más info, visitar:
// - https://archive.org/stream/PainlessCRC/crc_v3.txt
//   (en especial capitulos del 9 al 11)
// - https://www.w3.org/TR/2003/REC-PNG-20031110/
//   (punto 5.5, CRC de 32 bits para uso en PNG)
// - https://www.w3.org/TR/PNG-CRCAppendix.html
//   (ejemplo de código en C)
uint32_t PNGChunk::calculate_crc(uint8_t* stream, int streamLength) {
    uint32_t rem = 0xFFFFFFFF;
    for (int byte = 0; byte < streamLength; byte++) {
        rem = (rem >> 8) ^ this->CRC_TABLE[(rem ^ stream[byte]) & 0xFF];
    }
    return rem ^ 0xFFFFFFFF;
}

// Constructor (crear tabla para el cálculo de CRC si es necesario)
PNGChunk::PNGChunk() : length(0), chunkType(nullptr), data(nullptr), crc(0) {
    static bool crcTableCalculated = false;
    if (!crcTableCalculated) {
        for (int i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int bit = 0; bit < 8; bit++) {
                if (c & 1) {
                    c = (c >> 1) ^ this->CRC32_DIVISOR;
                } else {
                    c = (c >> 1);
                }
            }
            this->CRC_TABLE[i] = c;
        }
        crcTableCalculated = true;
    }
}

// Obtener datos del chunk a partir de un ifstream
// Devuelve true si la lectura es correcta, false si no
bool PNGChunk::read_file(std::ifstream& is) {
    if (!is.good()) {
        std::cerr << "An error ocurred while reading the data" << std::endl;
        return false;
    }

    // Longitud (4 bytes)
    uint8_t length_aux[4];
    is.read((char*)length_aux, 4);
    this->length = swap_bytes(length_aux);

    // Tipo y datos (4 bytes)
    this->chunkCrcDividend = new uint8_t[length + 4];
    is.read((char*)this->chunkCrcDividend, length + 4);
    this->chunkType = this->chunkCrcDividend;
    this->data = &this->chunkCrcDividend[4];

    // CRC
    uint8_t crc_aux[4];
    is.read((char*)crc_aux, 4);
    this->crc = swap_bytes(crc_aux);

    // Comprobar CRC correcto
    uint32_t calc_crc = calculate_crc(this->chunkCrcDividend, this->length + 4);
    if (calc_crc == this->crc) {
        if (this->is_type("IHDR")) {
            this->chunkInfo = new PNGChunk::IHDRInfo();
            this->chunkInfo->read_info(data, length);
        }
        return true;
    } else {
        std::cerr << "Incorrect chunk CRC" << std::endl;
        return false;
    }
}

// Comprobación del tipo de chunk
bool PNGChunk::is_type(const char* type) {
    // paso a string para añadir terminación
    std::string typeString((char*)this->chunkType, 4);
    return typeString.compare(type) == 0;
}

PNGChunk::~PNGChunk() { delete[] this->chunkCrcDividend; }

bool PNGChunk::IHDRInfo::read_info(uint8_t* data, uint8_t length) {
    this->width = swap_bytes(data);
    this->height = swap_bytes(&data[4]);
    this->bitDepth = data[8];
    this->colorType = data[9];
    this->compression = data[10];
    this->filter = data[11];
    this->interlace = data[12];
}
