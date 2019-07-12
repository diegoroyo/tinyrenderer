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

Vec3f barycentric(Vec2i t0, Vec2i t1, Vec2i t2, Vec2i p) {
    Vec2i ab = t1 - t0;
    Vec2i ac = t2 - t0;
    Vec2i pa = t0 - p;
    // Obtener u, v tq u*ab + v*ac + pa = 0
    // Resolver (u v 1)'*(abx acx pax) = 0 y (u v 1)'*(aby acy pay) = 0
    // producto vectorial de ambos dos, escalado para z = 1 (x/z, y/z, z/z=1)
    Vec3i cross = Vec3i(ab.x, ac.x, pa.x) ^ Vec3i(ab.y, ac.y, pa.y);
    // Si la componente z es 0, el triangulo es degenerado y no se dibuja
    if (cross.z == 0) return Vec3f(-1.0, -1.0, -1.0);
    float u = cross.x / (float)cross.z;
    float v = cross.y / (float)cross.z;
    return Vec3f(1.0 - u - v, u, v);
}

void triangle(Vec2i t0, Vec2i t1, Vec2i t2, PNGImage& image,
              const RGBColor& color) {
    // Bounding box (dos puntos (xmin, ymin), (xmax, ymax))
    Vec2i bboxmin(t0.x, t0.y), bboxmax(t0.x, t0.y);
    for (int i = 0; i < 2; i++) {
        if (t1.raw[i] < bboxmin.raw[i]) bboxmin.raw[i] = t1.raw[i];
        if (t2.raw[i] < bboxmin.raw[i]) bboxmin.raw[i] = t2.raw[i];
        if (t1.raw[i] > bboxmax.raw[i]) bboxmax.raw[i] = t1.raw[i];
        if (t2.raw[i] > bboxmax.raw[i]) bboxmax.raw[i] = t2.raw[i];
    }
    // Recortar el trozo de fuera de la imagen
    if (bboxmin.x < 0) bboxmin.x = 0;
    if (bboxmin.y < 0) bboxmin.y = 0;
    if (bboxmax.x > image.width - 1) bboxmax.x = image.width - 1;
    if (bboxmax.y > image.height - 1) bboxmax.y = image.height - 1;
    // Pintar los puntos de bbox que pertenecen al triangulo
    // es decir, bc_coords tiene todas las componentes positivas
    for (int y = bboxmin.y; y <= bboxmax.y; y++) {
        for (int x = bboxmin.x; x <= bboxmax.x; x++) {
            Vec3f bc_coords = barycentric(t0, t1, t2, Vec2i(x, y));
            if (bc_coords.x >= 0 && bc_coords.y >= 0 && bc_coords.z >= 0) {
                // El punto pertenece al triangulo
                image.set_pixel(x, y, color);
            }
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