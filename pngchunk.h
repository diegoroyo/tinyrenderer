#include <stdint.h>

#pragma once

// Una imagen PNG está formada por varios chunks de este tipo
// Cada chunk tiene la siguiente estructura:
//   4 bytes: longitud del fichero (Big Endian)
//   4 bytes: tipo de chunk (ver IHDR, PLTE, IDAT, IEND)
//   <longitud> bytes: datos del chunk
//   4 bytes: CRC
class PNGChunk {
   private:
    static const uint32_t CRC32_DIVISOR = 0xEDB88320;
    static uint32_t CRC_TABLE[256];
    uint8_t* chunkCrcDividend;

    uint32_t calculate_crc(uint8_t* stream, int streamLength);

   public:
    uint32_t length;
    uint8_t* chunkType;
    uint8_t* data;
    uint32_t crc;

    PNGChunk();
    ~PNGChunk();

    bool read_file(std::ifstream& is);
};