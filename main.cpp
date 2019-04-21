#include <iostream>
#include "pngimage.h"

void imageInfo(PNGImage& image) {
    std::cout << "Imagen de " << image.width << "x" << image.height
              << std::endl;
    for (int x = 0; x < image.width; x++) {
        for (int y = 0; y < image.height; y++) {
            std::cout << "Pixel (" << (int)x << ", " << (int)y << "): "
                      << "r = " << int(image.get_pixel(x, y)->r)
                      << ", g = " << int(image.get_pixel(x, y)->g)
                      << ", b = " << int(image.get_pixel(x, y)->b)
                      << std::endl;
        }
    }
}

int main() {
    PNGImage img;
    PNGImage::RGBColor white(255, 255, 255);
    if (img.read_png_file("png/test_tom.png")) {
        // Píxeles de la diagonal en blanco
        for (int i = 0; i < img.width; i++) {
            img.set_pixel(i, i, &white);
        }
        imageInfo(img);
    } else {
        std::cout << "Error al leer la imagen" << std::endl;
    }

    return 0;
}