#include <iostream>
#include "pngimage/pngimage.h"
#include "pngimage/rgbcolor.h"

void line(int x0, int y0, int x1, int y1, PNGImage& image,
          const RGBColor& color) {
    bool steep = false;
    if (std::abs(y0 - y1) > std::abs(x0 - x1)) {
        // más alta que ancha
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) {
        // dibujar de derecha a izquierda
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = y1 - y0;
    // multiplicar todos los errores por (2*dx)
    // derror pasa de abs(dy/dx) a abs(dy)*2
    int derror2 = std::abs(dy)*2;
    int error2 = 0;
    int y = y0;
    for (int x = x0; x <= x1; x++) {
        if (steep) {
            image.set_pixel(y, x, color);
        } else {
            image.set_pixel(x, y, color);
        }
        error2 += derror2;
        // necesario comprobar por .5 * 2 * dx = dx
        if (error2 > dx) {
            y += (y1 > y0) ? 1 : -1;
            // restar 1 * 2 * dx = 2*dx
            error2 -= 2*dx;
        }
    }
}

int main() {
    PNGImage image(100, 100, RGBColor::Black);
    line(13, 20, 80, 40, image, RGBColor::White);
    line(20, 13, 40, 80, image, RGBColor::Red);
    line(80, 40, 13, 20, image, RGBColor::Red);
    image.flip_vertically();
    image.write_png_file("images/output.png");
    return 0;
}