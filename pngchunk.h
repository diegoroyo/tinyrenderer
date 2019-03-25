#include <stdint.h>

#pragma once

class PNGChunk {
   private:
    static const uint32_t CRC32_DIVISOR = 0x04C11DB7;

   public:
    uint32_t length;
    uint8_t* chunkType;
    uint8_t* data;
    uint32_t crc;

    PNGChunk();
    ~PNGChunk();

    bool read_file(std::ifstream& is);
};