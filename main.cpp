#include <iostream>
#include "pngimage/pngimage.h"
#include "pngimage/rgbcolor.h"
#include "model.h"

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

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <model_name>" << std::endl;
        return 1;
    }

    Model model(argv[1]);
    const int width = 800, height = 800;
    PNGImage image(width, height, RGBColor::Black);

    // Renderizado básico
    for (int i = 0; i < model.nfaces(); i++) {
        std::vector<int> face = model.face(i);
        for (int j = 0; j < face.size(); j++) {
            Vec3f v0 = model.vert(face[j]);
            Vec3f v1 = model.vert(face[(j + 1) % face.size()]);
            int x0 = (v0.x + 1.0) * width / 2.0;
            int x1 = (v1.x + 1.0) * width / 2.0;
            int y0 = (v0.y + 1.0) * height / 2.0;
            int y1 = (v1.y + 1.0) * height / 2.0;
            line(x0, y0, x1, y1, image, RGBColor::White);
        }
    }

    image.flip_vertically();
    image.write_png_file("images/output.png");
    return 0;
}