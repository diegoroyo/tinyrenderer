#include <stdint.h>
#include <cstring>
#include <fstream>
#include <iostream>

#include "pngchunk.h"

uint32_t PNGChunk::CRC_TABLE[256];

// Convierte de Big Endian (archivo .png) al local de la máquina
uint32_t read_endian(uint8_t* read) {
    return read[3] | read[2] << 8 | read[1] << 16 | read[0] << 24;
}

// Convierte del local de la máquina a Big Endian (archivo .png)
void write_endian(uint32_t value, uint8_t* write) {
    write[0] = value >> 24 & 0xFF;
    write[1] = value >> 16 & 0xFF;
    write[2] = value >> 8 & 0xFF;
    write[3] = value & 0xFF;
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

// Constructor sin chunkInfo
PNGChunk::PNGChunk() : PNGChunk(nullptr) {}

// Crear tabla para el cálculo de CRC si es necesario
PNGChunk::PNGChunk(ChunkInfo* _chunkInfo)
    : chunkCrcDividend(nullptr),
      length(0),
      chunkType(nullptr),
      data(nullptr),
      crc(0),
      chunkInfo(_chunkInfo) {
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
    this->length = read_endian(length_aux);

    // Tipo y datos (4 bytes)
    this->chunkCrcDividend = new uint8_t[length + 4];
    is.read((char*)this->chunkCrcDividend, length + 4);
    this->chunkType = this->chunkCrcDividend;
    this->data = &this->chunkCrcDividend[4];

    // CRC
    uint8_t crc_aux[4];
    is.read((char*)crc_aux, 4);
    this->crc = read_endian(crc_aux);

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
            uint32_t calc =
                info->adler_checksum(info->blockData, info->blockLength);
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

bool PNGChunk::write_file(std::ofstream& os) {
    if (this->chunkInfo == nullptr) {
        std::cerr << "Attempted to write to file without data" << std::endl;
        return false;
    }
    uint8_t* data;
    uint32_t dataLength;
    // Comprobar información correcta (al menos 8 bytes)
    if (!chunkInfo->get_writable_info(data, dataLength) || dataLength < 8) {
        return false;
    }
    os.write((char*)data, dataLength);
    // Calcular CRC de los datos sin length
    uint32_t calc_crc = calculate_crc(&data[4], dataLength - 4);
    uint8_t endian_crc[4];
    write_endian(calc_crc, endian_crc);
    os.write((char*)&endian_crc, 4);
    delete[] data;  // ya no se emplea
    return true;
}

// Comprobación del tipo de chunk
bool PNGChunk::is_type(const char* type) {
    // paso a string para añadir terminación
    std::string typeString((char*)this->chunkType, 4);
    return typeString.compare(type) == 0;
}

PNGChunk::~PNGChunk() {
    if (this->chunkCrcDividend) {
        delete[] this->chunkCrcDividend;
    }
    if (this->chunkInfo) {
        delete this->chunkInfo;
    }
}

PNGChunk::IHDRInfo::IHDRInfo(int _width, int _height)
    : width(_width),
      height(_height),
      bitDepth(IHDR_BIT_DEPTH),
      colorType(IHDR_COLOR_TYPE),
      compression(IHDR_COMPRESSION),
      filter(IHDR_FILTER),
      interlace(IHDR_INTERLACE) {}

bool PNGChunk::IHDRInfo::is_supported() {
    return this->bitDepth == IHDR_BIT_DEPTH &&
           this->colorType == IHDR_COLOR_TYPE &&
           this->compression == IHDR_COMPRESSION &&
           this->filter == IHDR_COMPRESSION &&
           this->interlace == IHDR_INTERLACE;
}

// Más info:
// http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
bool PNGChunk::IHDRInfo::read_info(uint8_t* data, uint32_t length) {
    if (length != IHDR_LENGTH) {
        std::cerr << "IHDR chunk has wrong length" << std::endl;
        return false;
    }
    this->width = read_endian(data);
    this->height = read_endian(&data[4]);
    this->bitDepth = data[8];
    this->colorType = data[9];
    this->compression = data[10];
    this->filter = data[11];
    this->interlace = data[12];
    return true;
}

bool PNGChunk::IHDRInfo::get_writable_info(uint8_t*& data, uint32_t& length) {
    length = IHDR_LENGTH + 8;    // 8 bytes (4 length + 4 type)
    data = new uint8_t[length];  // borrado en write_file
    write_endian(IHDR_LENGTH, data);
    memcpy(&data[4], &"IHDR", 4);
    write_endian(this->width, &data[8]);
    write_endian(this->height, &data[12]);
    data[16] = this->bitDepth;
    data[17] = this->colorType;
    data[18] = this->compression;
    data[19] = this->filter;
    data[20] = this->interlace;
    return true;
}

PNGChunk::IDATInfo::IDATInfo(int width, int height, RGBColor*** pixels)
    : pixelData(pixels) {
    // 24 bits per pixel RGB + tipo de filtro al inicio de cada fila
    uint16_t pixelLength = (3 * width + 1) * height;
    // Añadir cabecera
    this->blockLength = IDAT_LENGTH_HEADER + pixelLength + IDAT_LENGTH_CHECKSUM;
    this->blockData = new uint8_t[this->blockLength];
    // ZLIB Header
    this->blockData[0] = IDAT_ZLIB_HEADER >> 8 & 0xFF;
    this->blockData[1] = IDAT_ZLIB_HEADER & 0xFF;
    // Block header
    this->blockData[2] = IDAT_BLOCK_HEADER;
    // Longitud y su invertido Ca1
    this->blockData[3] = pixelLength & 0xFF;
    this->blockData[4] = pixelLength >> 8 & 0xFF;
    this->blockData[5] = ~pixelLength & 0xFF;
    this->blockData[6] = ~pixelLength >> 8 & 0xFF;
    int i = 7;  // posición de pixel
    for (int y = 0; y < height; y++) {
        blockData[i++] = 0;  // tipo de filtro 0 (sin filtro)
        for (int x = 0; x < width; x++) {
            blockData[i++] = pixels[y][x]->r;
            blockData[i++] = pixels[y][x]->g;
            blockData[i++] = pixels[y][x]->b;
        }
    }
    write_endian(adler_checksum(&this->blockData[7], pixelLength),
                 &this->blockData[this->blockLength - 4]);
}

// Cálculo del checksum Adler-32:
// https://en.wikipedia.org/wiki/Adler-32
uint32_t PNGChunk::IDATInfo::adler_checksum(uint8_t* data, uint32_t length) {
    int a = 1;
    int b = 0;
    for (int i = 0; i < length; i++) {
        a = (a + data[i]) % ADLER_MODULO;
        b = (b + a) % ADLER_MODULO;
    }
    return b * 65536 + a;
}

// Modo de filtrado #4, mira los pixeles a la izquierda y derecha
// a: izquierda, b: arriba, c: arriba a la izquierda
uint8_t PNGChunk::IDATInfo::paeth_pred(uint8_t a, uint8_t b, uint8_t c) {
    int p = a + b - c;                    // estimación inicial
    int pa = p - a >= 0 ? p - a : a - p;  // abs(p - a)
    int pb = p - b >= 0 ? p - b : b - p;  // abs(p - b)
    int pc = p - c >= 0 ? p - c : c - p;  // abs(p - c)
    // devolver más cercano a a, b, c
    // en caso de empate, a > b > c
    return pa <= pb && pa <= pc ? a : (pb <= pc ? b : c);
}

// Datos descomprimidos de un chunk IDAT, añadir a la imagen
// https://stackoverflow.com/questions/49017937/png-decompressed-idat-chunk-how-to-read
bool PNGChunk::IDATInfo::process_pixels(int width, int height) {
    int i = 0;                   // posición de memoria apuntada
    int pixelX = 0, pixelY = 0;  // pixeles X, Y apuntados
    int filterType = 0;          // tipo de filtro para la fila en curso
    bool readOk = true;
    RGBColor blackPixel(0, 0, 0);  // TODO usar constantes de RGBColor
    this->pixelData = new RGBColor**[height];
    while (readOk && i < this->blockLength) {
        // Cada fila comienza con un byte para indicar tipo de filtro
        // Solo se soporta tipos 0, 1 y 2 (None, Sub y Add)
        // https://www.w3.org/TR/PNG-Filters.html
        if (pixelX == 0) {
            if (this->blockData[i] > 4) {
                std::cerr << "Error: invalid filter type" << std::endl;
                readOk = false;
            } else {
                this->pixelData[pixelY] = new RGBColor*[width];
                filterType = this->blockData[i];
                i++;  // leido un byte
            }
        }
        // Leer datos del pixel (r, g, b 3 bytes en ese orden)
        if (readOk) {
            uint8_t r = this->blockData[i], g = this->blockData[i + 1],
                    b = this->blockData[i + 2];
            const RGBColor *left, *top, *leftTop;
            left =
                pixelX > 0 ? this->pixelData[pixelY][pixelX - 1] : &blackPixel;
            top =
                pixelY > 0 ? this->pixelData[pixelY - 1][pixelX] : &blackPixel;
            leftTop = pixelX > 0 && pixelY > 0
                          ? this->pixelData[pixelY - 1][pixelX - 1]
                          : &blackPixel;
            switch (filterType) {
                case 1:  // Sub(x) = Raw(x) - Raw(x-bpp)
                    r += left->r;
                    g += left->g;
                    b += left->b;
                    break;
                case 2:  // Up(x) = Raw(x) - Prior(x)
                    r += top->r;
                    g += top->g;
                    b += top->b;
                    break;
                case 3:  // Average(x) = Raw(x) - floor((Raw(x-bpp)+Prior(x))/2)
                    r += (left->r + top->r) / 2;
                    g += (left->g + top->g) / 2;
                    b += (left->b + top->b) / 2;
                    break;
                case 4:  // Paeth(x) = Raw(x) - PaethPredictor(Raw(x-bpp),
                         //                        Prior(x), Prior(x-bpp))
                    r += paeth_pred(left->r, top->r, leftTop->r);
                    g += paeth_pred(left->g, top->g, leftTop->g);
                    b += paeth_pred(left->b, top->b, leftTop->b);
                    break;
            }
            this->pixelData[pixelY][pixelX] = new RGBColor(r, g, b);
            i = i + 3;  // leidos 3 bytes
            pixelX++;
            if (pixelX == width) {
                pixelX = 0;
                pixelY++;
            }
        }
    }
    return readOk;
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
    this->checksum = read_endian(&data[chunkLength - 4]);
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
    if (static_cast<uint16_t>(compressionHeader & 0x0FE0) != IDAT_ZLIB_HEADER &
        0x0FE0) {
        std::cerr << "Unsupported IDAT chunk compression type" << std::endl;
        return false;
    }

    // Cabecera del bloque (3 bits + 5 sin usar: 1 byte)
    uint8_t blockHeader = data[2];
    // Un bloque (BFINAL = 1) sin comprimir (BTYPE = 00), el resto del byte
    // no contiene información (todo 0)
    if (blockHeader != IDAT_BLOCK_HEADER) {
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

bool PNGChunk::IDATInfo::get_writable_info(uint8_t*& data, uint32_t& length) {
    length = this->blockLength + 8;
    data = new uint8_t[length];
    write_endian(this->blockLength, data);
    memcpy(&data[4], &"IDAT", 4);
    // Copiar y borrar no es la opción más rápida, pero es simple
    memcpy(&data[8], this->blockData, this->blockLength);
    delete[] this->blockData;
    return true;
}

bool PNGChunk::IENDInfo::read_info(uint8_t* data, uint32_t length) {
    return true;
}

bool PNGChunk::IENDInfo::get_writable_info(uint8_t*& data, uint32_t& length) {
    length = 8;                  // 8 bytes (4 length + 4 type)
    data = new uint8_t[length];  // borrado en write_file
    write_endian(0, data);
    memcpy(&data[4], &"IEND", 4);
    return true;
}