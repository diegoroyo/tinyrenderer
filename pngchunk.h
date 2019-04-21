#pragma once

#include <stdint.h>
#include "rgbcolor.h"

// Una imagen PNG está formada por varios chunks de este tipo
// http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
// Cada chunk tiene la siguiente estructura:
//   4 bytes: longitud del fichero (Big Endian)
//   4 bytes: tipo de chunk (ver IHDR, PLTE, IDAT, IEND)
//   <longitud> bytes: datos del chunk
//   4 bytes: CRC
class PNGChunk {
   private:
    static const int ADLER_MODULO = 65521;             // ver adler_checksum
    static const uint32_t CRC32_DIVISOR = 0xEDB88320;  // ver calculate_crc
    static uint32_t CRC_TABLE[256];
    uint8_t* chunkCrcDividend;

    uint32_t adler_checksum(uint8_t* data, uint32_t length);
    uint32_t calculate_crc(uint8_t* stream, int streamLength);
    bool read_data(std::ifstream& is);

   public:
    // Posibilidad de ampliar a más tipos: PLTE, etc.
    class ChunkInfo {
       public:
        virtual bool read_info(uint8_t* data, uint32_t length) = 0;
        virtual void write_file(std::ofstream& os) = 0;
    };

    // Información principal de la imagen
    class IHDRInfo : public ChunkInfo {
       private:
        // Bytes de información (ver campos públicos)
        static const int IHDR_LENGTH = 13;
        // Valores soportados
        static const int IHDR_BIT_DEPTH = 8;
        static const int IHDR_COLOR_TYPE = 2;
        static const int IHDR_COMPRESSION = 0;
        static const int IHDR_FILTER = 0;
        static const int IHDR_INTERLACE = 0;

       public:
        uint32_t width, height;
        uint8_t bitDepth, colorType, compression, filter, interlace;

        IHDRInfo() = default;
        IHDRInfo(int _width, int _height);
        bool is_supported();
        bool read_info(uint8_t* data, uint32_t length) override;
        void write_file(std::ofstream& os) override;
    };

    // Datos (colores) de la imagen
    class IDATInfo : public ChunkInfo {
       public:
        // Tamaño mínimo (headers sin datos)
        // 2 header + 5 block header | 4 block length
        static const int IDAT_LENGTH_HEADER = 7;
        static const int IDAT_LENGTH_CHECKSUM = 4;

        uint8_t* blockData;
        uint16_t blockLength;  // todos los datos IDAT
        uint16_t chunkLength;  // solo los de un chunk
        uint32_t checksum;     // solo valido para el ultimo chunk

        IDATInfo() = default;
        bool set_data(uint8_t* data, uint32_t blockLength,
                      uint32_t chunkLength);
        bool read_info(uint8_t* data, uint32_t length) override;
        void write_file(std::ofstream& os) override;
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