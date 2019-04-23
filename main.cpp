#include <iostream>
#include "pngimage.h"
#include "rgbcolor.h"

void imageInfo(PNGImage& image) {
    std::cout << "Imagen de " << image.width << "x" << image.height
              << std::endl;
    for (int y = 0; y < image.height; y++) {
        for (int x = 0; x < image.width; x++) {
            std::cout << "Pixel (" << (int)x << ", " << (int)y << "): "
                      << "r = " << int(image.get_pixel(x, y)->r)
                      << ", g = " << int(image.get_pixel(x, y)->g)
                      << ", b = " << int(image.get_pixel(x, y)->b) << std::endl;
        }
    }
}

int main() {
    PNGImage img;
    RGBColor green(0, 255, 0);
    if (img.read_png_file("png/test_tom.png")) {
        // Mostrar información
        imageInfo(img);
        // Píxeles de la diagonal en verde
        for (int i = 0; i < img.width; i++) {
            img.set_pixel(i, i, &green);
        }
        img.write_png_file("png/test_tom_other.png");
    } else {
        std::cout << "Error al leer la imagen" << std::endl;
    }

    return 0;
}