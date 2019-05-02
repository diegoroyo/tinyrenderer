#include <math.h>
#include <iostream>
#include "pngimage.h"
#include "rgbcolor.h"

void imageInfo(PNGImage& image) {
    std::cout << "Imagen de " << image.width << "x" << image.height
              << std::endl;
    RGBColor pixel;
    for (int y = 0; y < image.height; y++) {
        for (int x = 0; x < image.width; x++) {
            image.get_pixel(x, y, pixel);
            std::cout << "Pixel (" << (int)x << ", " << (int)y << "): "
                      << "r = " << int(pixel.r) << ", g = " << int(pixel.g)
                      << ", b = " << int(pixel.b) << std::endl;
        }
    }
}

int main() {
    /*
    PNGImage img;
    if (img.read_png_file("png/test_tom.png")) {
        // Mostrar información
        imageInfo(img);
        // Píxeles de la diagonal en verde
        for (int i = 0; i < img.width; i++) {
            img.set_pixel(i, i, RGBColor::Green);
        }
        img.write_png_file("png/test_tom_other.png");
    } else {
        std::cout << "Error al leer la imagen" << std::endl;
    }
    */

    /*
    RGBColor grey(200, 200, 200);
    PNGImage img(100, 10, grey);
    for (int x = 0; x < img.width; x++) {
        for (int y = 0; y < img.height; y++) {
            double v = 5.0 * sin(x / 5.0) + 5.0;
            if (v > y && v < y + 1) {
                img.set_pixel(x, y, RGBColor::Red);
            }
        }
    }
    img.write_png_file("png/sin_graph.png");
    */

    PNGImage img(500, 500);
    for (int y = 0; y < img.height; y++) {
        for (int x = 0; x < img.width; x++) {
            RGBColor color(y * 255 / img.height, x * 255 / img.width, 0);
            img.set_pixel(x, y, color);
        }
    }
    img.write_png_file("png/gradient.png");

    return 0;
}