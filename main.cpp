#include <iostream>
#include "model.h"
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
    int derror2 = std::abs(dy) * 2;
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
            error2 -= 2 * dx;
        }
    }
}

void line(Vec2i p0, Vec2i p1, PNGImage& image, const RGBColor& color) {
    line(p0.x, p0.y, p1.x, p1.y, image, color);
}

void triangle(Vec2i t0, Vec2i t1, Vec2i t2, PNGImage& image,
              const RGBColor& color) {
    // Ordenar vertices de menor a mayor y
    if (t0.y > t1.y) std::swap(t0, t1);
    if (t0.y > t2.y) std::swap(t0, t2);
    if (t1.y > t2.y) std::swap(t1, t2);
    line(t0, t1, image, color);
    line(t0, t2, image, color);
    line(t1, t2, image, color);
    // t0 está abajo, t2 arriba
    // lineas horizontales desde t0 hasta t1
    int x0 = t0.x, x1 = t0.x;
    int error0 = 0, error1 = 0;
    int derror0 = std::abs(t1.x - t0.x) * 2,
        derror1 = std::abs(t2.x - t0.x) * 2;
    for (int y = t0.y; y < t1.y; y++) {
        line(x0, y, x1, y, image, color);
        error0 += derror0;
        while (error0 > t1.y - t0.y) {
            x0 += (t1.x > t0.x) ? 1 : -1;
            error0 -= 2 * (t1.y - t0.y);
        }
        error1 += derror1;
        while (error1 > t2.y - t0.y) {
            x1 += (t2.x > t0.x) ? 1 : -1;
            error1 -= 2 * (t2.y - t0.y);
        }
    }
    derror0 = std::abs(t2.x - t1.x) * 2;
    error0 = 0;
    for (int y = t1.y; y <= t2.y; y++) {
        line(x0, y, x1, y, image, color);
        error0 += derror0;
        while (error0 > t2.y - t1.y) {
            x0 += (t2.x > t1.x) ? 1 : -1;
            error0 -= 2 * (t2.y - t1.y);
        }
        error1 += derror1;
        while (error1 > t2.y - t0.y) {
            x1 += (t2.x > t0.x) ? 1 : -1;
            error1 -= 2 * (t2.y - t0.y);
        }
    }
}

int main(int argc, char** argv) {
    PNGImage image(200, 200, RGBColor::Black);
    Vec2i t0[3] = {Vec2i(10, 70), Vec2i(50, 160), Vec2i(70, 80)};
    Vec2i t1[3] = {Vec2i(180, 50), Vec2i(150, 1), Vec2i(70, 180)};
    Vec2i t2[3] = {Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180)};
    triangle(t0[0], t0[1], t0[2], image, RGBColor::Red);
    triangle(t1[0], t1[1], t1[2], image, RGBColor::White);
    triangle(t2[0], t2[1], t2[2], image, RGBColor::Green);

    image.flip_vertically();
    image.write_png_file("images/output.png");
    return 0;
}