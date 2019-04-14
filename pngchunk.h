#include <stdint.h>

#pragma once

// Una imagen PNG está formada por varios chunks de este tipo
// http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
// Cada chunk tiene la siguiente estructura:
//   4 bytes: longitud del fichero (Big Endian)
//   4 bytes: tipo de chunk (ver IHDR, PLTE, IDAT, IEND)
//   <longitud> bytes: datos del chunk
//   4 bytes: CRC
class PNGChunk {
   private:
    static const uint32_t CRC32_DIVISOR = 0xEDB88320; // ver calculate_crc
    static uint32_t CRC_TABLE[256];
    uint8_t* chunkCrcDividend;

    uint32_t calculate_crc(uint8_t* stream, int streamLength);

   public:
    // Posibilidad de ampliar a más tipos: PLTE, etc.
    class ChunkInfo {
       public:
        virtual bool read_info(uint8_t* data, uint8_t length) = 0;
    };

    // Información principal de la imagen
    class IHDRInfo : public ChunkInfo {
       private:
        // Bytes de información (ver campos públicos)
        static const int IHDR_LENGTH = 13;

       public:
        uint32_t width, height;
        uint8_t bitDepth, colorType, compression, filter, interlace;

        bool read_info(uint8_t* data, uint8_t length) override;
    };

    // Datos (colores) de la imagen
    class IDATInfo : public ChunkInfo {
       private:
        // Tamaño mínimo (headers sin datos)
        // 2 header + 1 block header + 4 block length
        //   (+ datos bloque) + 4 adler32 al final
        static const int IDAT_LENGTH = 11;
        static const int ADLER_MODULO = 65521; // ver adler_checksum

        uint32_t adler_checksum(uint8_t* data, uint8_t length);
       public:
        uint8_t* data;
        uint16_t length;

        bool read_info(uint8_t* data, uint8_t length) override;
    };

    uint32_t length;
    uint8_t* chunkType;
    uint8_t* data;
    uint32_t crc;
    ChunkInfo* chunkInfo;

    PNGChunk();
    ~PNGChunk();

    bool read_file(std::ifstream& is);
    bool is_type(const char* type);
};