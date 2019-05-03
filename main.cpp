#include <iostream>
#include "pngimage/pngimage.h"
#include "pngimage/rgbcolor.h"

int main() {
    // Crear una imagen desde 0 y guardarla (gradient.png)
    PNGImage newImg(256, 256);
    for (int y = 0; y < newImg.height; y++) {
        for (int x = 0; x < newImg.width; x++) {
            RGBColor color(y * 255 / newImg.height, x * 255 / newImg.width, 0);
            newImg.set_pixel(x, y, color);
        }
    }
    newImg.write_png_file("pngimage/gradient.png");

    // Leer gradient.png y generar su negativo
    PNGImage readImg;
    if (readImg.read_png_file("pngimage/gradient.png")) {
        RGBColor readColor;
        for (int y = 0; y < readImg.height; y++) {
            for (int x = 0; x < readImg.width; x++) {
                readImg.get_pixel(x, y, readColor);
                readColor.r = 255 - readColor.r;
                readColor.g = 255 - readColor.g;
                readColor.b = 255 - readColor.b;
                readImg.set_pixel(x, y, readColor);
            }
        }
        readImg.write_png_file("pngimage/gradient_negative.png");
    } else {
        std::cout << "Error al leer la imagen" << std::endl;
    }

    return 0;
}