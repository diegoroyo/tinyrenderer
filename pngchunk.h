#include <stdint.h>

#pragma once

// Una imagen PNG está formada por varios chunks de este tipo
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