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

// Cálculo del checksum Adler-32:
// https://en.wikipedia.org/wiki/Adler-32
uint32_t PNGChunk::adler_checksum(uint8_t* data, uint32_t length) {
    int a = 1;
    int b = 0;
    for (int i = 0; i < length; i++) {
        a = (a + data[i]) % ADLER_MODULO;
        b = (b + a) % ADLER_MODULO;
    }
    return b * 65536 + a;
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
PNGChunk::PNGChunk()
    : length(0), chunkType(nullptr), data(nullptr), crc(0), chunkInfo(nullptr) {
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
bool PNGChunk::read_data(std::ifstream& is) {
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
    if (calc_crc != this->crc) {
        std::cerr << "Incorrect chunk CRC" << std::endl;
        return false;
    } else {
        return true;
    }
}

// Obtener datos del chunk a partir de un ifstream
// Devuelve true si la lectura es correcta, false si no
bool PNGChunk::read_file(std::ifstream& is) {
    // Leer ifstream
    bool isChunkOk = this->read_data(is);
    if (!isChunkOk) {
        return false;
    }

    // Tratar los datos del chunk según su tipo
    if (this->is_type("IHDR")) {
        this->chunkInfo = new PNGChunk::IHDRInfo();
        return this->chunkInfo->read_info(this->data, this->length);

    } else if (this->is_type("IDAT")) {
        this->chunkInfo = new PNGChunk::IDATInfo();
        PNGChunk::IDATInfo* info =
            dynamic_cast<PNGChunk::IDATInfo*>(this->chunkInfo);
        isChunkOk = info->read_info(this->data, this->length);
        if (!isChunkOk) {
            return false;
        }

        // La información puede ir separada en varios chunks IDAT
        if (info->chunkLength < info->blockLength) {
            // Crear un buffer más grande para guardar los chunks IDAT
            uint8_t* firstChunkData = info->blockData;
            info->blockData = new uint8_t[info->blockLength];
            memcpy(info->blockData, firstChunkData, info->chunkLength);
            // no hace falta borrar firstChunkData, ya lo borra este chunk

            // Leer el resto de chunks IDAT (tienen que ir seguidos)
            while (isChunkOk && info->chunkLength < info->blockLength) {
                // Leer chunk siguiente
                PNGChunk moreChunk;
                isChunkOk = moreChunk.read_data(is);
                if (!isChunkOk || !moreChunk.is_type("IDAT")) {
                    std::cerr << "Error: Invalid chunk after first IDAT chunk"
                              << std::endl;
                    return false;
                }

                // Almacenar datos en su chunkInfo
                moreChunk.chunkInfo = new PNGChunk::IDATInfo();
                PNGChunk::IDATInfo* moreInfo =
                    dynamic_cast<PNGChunk::IDATInfo*>(moreChunk.chunkInfo);
                moreInfo->set_data(moreChunk.data,
                                   info->blockLength - info->chunkLength,
                                   moreChunk.length);

                // Juntar los datos del leido con la que tenemos
                memcpy(&info->blockData[info->chunkLength], moreInfo->blockData,
                       moreInfo->chunkLength);
                info->chunkLength = info->chunkLength + moreInfo->chunkLength;
                info->checksum = moreInfo->checksum;
            }
        }
        if (isChunkOk) {
            // Comprobar el checksum del bloque IDAT entero (todos los chunks)
            uint32_t calc = adler_checksum(info->blockData, info->blockLength);
            if (info->checksum != calc) {
                std::cerr << "Invalid IDAT block checksum" << std::endl;
                return false;
            }
        }
        return isChunkOk;

    } else {
        // No hace falta tratar nada para este tipo
        return true;
    }
}

// Comprobación del tipo de chunk
bool PNGChunk::is_type(const char* type) {
    // paso a string para añadir terminación
    std::string typeString((char*)this->chunkType, 4);
    return typeString.compare(type) == 0;
}

PNGChunk::~PNGChunk() {
    delete[] this->chunkCrcDividend;
    if (chunkInfo) {
        delete this->chunkInfo;
    }
}

// Más info:
// http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
bool PNGChunk::IHDRInfo::read_info(uint8_t* data, uint32_t length) {
    if (length != IHDR_LENGTH) {
        std::cerr << "IHDR chunk has wrong length" << std::endl;
        return false;
    }
    this->width = swap_bytes(data);
    this->height = swap_bytes(&data[4]);
    this->bitDepth = data[8];
    this->colorType = data[9];
    this->compression = data[10];
    this->filter = data[11];
    this->interlace = data[12];
    return true;
}

// Leer solo la parte de datos y un checksum que solo es valido si este es el
// último chunk
bool PNGChunk::IDATInfo::set_data(uint8_t* data, uint32_t blockLength,
                                  uint32_t chunkLength) {
    if (chunkLength < IDAT_LENGTH_CHECKSUM) {
        std::cerr << "IDAT chunk has wrong length" << std::endl;
        return false;
    }

    // Checksum Adler-32 de los datos (data, length)
    this->blockData = data;
    this->blockLength = blockLength;
    if (blockLength == chunkLength - IDAT_LENGTH_CHECKSUM) {
        this->chunkLength = blockLength;
    } else {
        this->chunkLength = chunkLength;
    }
    this->checksum = swap_bytes(&data[chunkLength - 4]);
    return true;
}

// Más info:
// https://stackoverflow.com/questions/33535388/deflate-compression-spec-clarifications
// https://github.com/libyal/assorted/blob/master/documentation/Deflate%20(zlib)%20compressed%20data%20format.asciidoc
bool PNGChunk::IDATInfo::read_info(uint8_t* data, uint32_t length) {
    if (length < IDAT_LENGTH_HEADER) {
        std::cerr << "IDAT chunk has wrong length" << std::endl;
        return false;
    }

    // Cabecera (2 bytes)
    uint16_t compressionHeader = data[0] << 8 | data[1];
    if (compressionHeader % 31 != 0) {
        std::cerr << "IDAT chunk has invalid CHECK bits" << std::endl;
        return false;
    }
    // Comprobar que CM = 8, FLEVEL = 0 y FDICT = 0
    if (static_cast<uint16_t>(compressionHeader & 0x0FE0) != 0x0800) {
        std::cerr << "Unsupported IDAT chunk compression type" << std::endl;
        return false;
    }

    // Cabecera del bloque (3 bits + 5 sin usar: 1 byte)
    uint8_t blockHeader = data[2];
    // Un bloque (BFINAL = 1) sin comprimir (BTYPE = 00), el resto del byte
    // no contiene información (todo 0)
    if (blockHeader != 0x01) {
        std::cerr << "Unsupported IDAT block type" << std::endl;
        return false;
    }

    // Longitud del bloque (2 bytes) + invertido Ca1 (2 bytes)
    uint16_t blockLength = data[4] << 8 | data[3];
    uint16_t invertedLength = data[6] << 8 | data[5];
    if (blockLength & invertedLength != 0) {
        std::cerr << "Invalid IDAT block length" << std::endl;
        return false;
    }

    return set_data(&data[7], blockLength, length - IDAT_LENGTH_HEADER);
}
