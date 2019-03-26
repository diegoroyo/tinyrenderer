#include <stdint.h>
#include <cstring>
#include <fstream>
#include <iostream>

#include <bitset>

#include "pngchunk.h"

// Corregir el valor leido en el archivo .png (Big Endian)
uint32_t swap_bytes(uint8_t* read) {
    return read[3] | read[2] << 8 | read[1] << 16 | read[0] << 24;
}

// Dar la vuelta a un conjunto de bits (0101 -> 1010)
template <std::size_t N>
void reverse(std::bitset<N>& b) {
    for (std::size_t i = 0; i < N / 2; ++i) {
        bool t = b[i];
        b[i] = b[N - i - 1];
        b[N - i - 1] = t;
    }
}

// Cálculo del CRC de 32 bits de un chunk de la imágen PNG
// Para más info, visitar:
// - https://archive.org/stream/PainlessCRC/crc_v3.txt
// - https://www.w3.org/TR/PNG-CRCAppendix.html
uint32_t PNGChunk::calculate_crc(uint8_t* stream, int streamLength) {
    /// TODO calcular la tabla solo una vez
    uint32_t crc_table[256]{0};
    for (int i = 0; i < 256; i++) {
        uint32_t c = i << 24;
        for (int bit = 7; bit >= 0; bit--) {
            if (c & 0x80000000) {
                c = (c << 1) ^ this->CRC32_DIVISOR;
            } else {
                c = (c << 1);
            }
        }
        crc_table[i] = c;
    }

    // TODO usar el algoritmo real (invertido)
    uint32_t rem = 0xFFFFFFFF;
    for (int byte = 0; byte < streamLength; byte++) {
        std::bitset<8> streamByte(stream[byte]);
        reverse(streamByte);
        uint8_t reversedSB = streamByte.to_ulong();
        rem = (rem << 8) ^ crc_table[((rem >> 24) ^ reversedSB) & 0xFF];
    }
    std::bitset<32> result(rem);
    reverse(result);
    rem = result.to_ulong();
    return rem ^ 0xFFFFFFFF;
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
    std::cout << "Length: " << length << std::endl;

    // Tipo y datos (4 bytes)
    this->crcDividend = new uint8_t[length + 4];
    is.read((char*)crcDividend, length + 4);
    this->chunkType = crcDividend;
    this->data = &crcDividend[4];

    // CRC
    uint8_t crc_aux[4];
    is.read((char*)crc_aux, 4);
    this->crc = swap_bytes(crc_aux);
    std::cout << "CRC: " << crc << std::endl;

    // Comprobar CRC correcto
    uint32_t calc_crc = calculate_crc(this->crcDividend, this->length + 4);

    if (calc_crc == this->crc) {
        return true;
    } else {
        std::cerr << "Incorrect chunk CRC\n";
        return false;
    }
}

PNGChunk::~PNGChunk() { delete[] crcDividend; }