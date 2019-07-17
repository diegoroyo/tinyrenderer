#include <iostream>
#include <limits>
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

// coord. baricéntricas de p dentro del triangulo t0-t1-t2
Vec3f barycentric(Vec3f t0, Vec3f t1, Vec3f t2, Vec3f p) {
    Vec3f ab = t1 - t0;
    Vec3f ac = t2 - t0;
    Vec3f pa = t0 - p;
    // Obtener u, v tq u*ab + v*ac + pa = 0
    // Resolver (u v 1)'*(abx acx pax) = 0 y (u v 1)'*(aby acy pay) = 0
    // producto vectorial de ambos dos, escalado para z = 1 (x/z, y/z, z/z=1)
    Vec3f cross = Vec3f(ab.x, ac.x, pa.x) ^ Vec3f(ab.y, ac.y, pa.y);
    // Pertenece si las coordenadas (u, v, 1-u-v) son todas mayores que 0
    // u = cross.x / cross.z, v = cross.y / cross.z
    // Es decir, las componentes x, y, z tienen el mismo signo (para u, v)
    // y (x + y) <= z (para 1-u-v)
    // También, si la z es 0 el triángulo es degenerado y no se dibuja
    Vec3f coords(-1.0f, -1.0f, -1.0f);
    if (std::abs(cross.z) > 0.01f) {  // solo si no es degenerado
        coords.y = cross.x / cross.z;
        coords.z = cross.y / cross.z;
        // se suma una pequeña constante para evitar errores de precisión
        coords.x = 1.0f + 1e-4f - (cross.x + cross.y) / cross.z;
    }
    return coords;
}

void triangle(Vec3f* t, Vec2f* uvs, Vec3f* norms, Model& model, PNGImage& image,
              const Vec3f light, float* zbuffer) {
    // Bounding box (dos puntos (xmin, ymin), (xmax, ymax))
    Vec2i bboxmin(t[0].x, t[0].y), bboxmax(t[0].x, t[0].y);
    for (int i = 0; i < 2; i++) {
        if (t[1].raw[i] < bboxmin.raw[i]) bboxmin.raw[i] = t[1].raw[i];
        if (t[2].raw[i] < bboxmin.raw[i]) bboxmin.raw[i] = t[2].raw[i];
        if (t[1].raw[i] > bboxmax.raw[i]) bboxmax.raw[i] = t[1].raw[i];
        if (t[2].raw[i] > bboxmax.raw[i]) bboxmax.raw[i] = t[2].raw[i];
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
            // Ver si el punto pertenece al triangulo
            Vec3f bc_coords = barycentric(t[0], t[1], t[2], Vec3f(x, y, 0.0f));
            if (bc_coords.x >= 0 && bc_coords.y >= 0 && bc_coords.z >= 0) {
                // zbuffer para dibujar lo más cercano a la cámara
                float z = bc_coords * Vec3f(t[0].z, t[1].z, t[2].z);
                if (zbuffer[x + y * image.width] < z) {
                    zbuffer[x + y * image.width] = z;
                    // Calcular el color (pixel x-y) de la textura
                    // y multiplicarlo por la intensidad (luz)
                    Vec2f uv;
                    Vec3f norm;
                    for (int i = 0; i < 3; i++) {
                        uv = uv + uvs[i] * bc_coords.raw[i];
                        norm = norm + norms[i] * bc_coords.raw[i];
                    }
                    float intensity = norm * light;
                    if (intensity < 0) intensity = 0;
                    image.set_pixel(x, y, RGBColor::White * intensity);
                }
            }
        }
    }
}

Matrix v2m(Vec3f v) {
    Matrix mat(4, 1);
    mat[0][0] = v.x;
    mat[1][0] = v.y;
    mat[2][0] = v.z;
    mat[3][0] = 1.0f;
    return mat;
}

Vec3f m2v(Matrix m) {
    return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <model_name>" << std::endl;
        return 1;
    }

    Model model(argv[1]);
    int width = 800, height = 800;
    PNGImage image(width, height, RGBColor::Black);

    Vec3f light(0, 0, 1);
    light.normalize();
    Vec3f camera(0, 0, 3.0f);
    Matrix perspective = Matrix::identity(4);
    //perspective[3][2] = -1.0f / camera.z;

    // Inicializar z-buffer a numeros negativos
    float zbuffer[height * width];
    for (int i = 0; i < height * width; i++) {
        zbuffer[i] = -1.0f * std::numeric_limits<float>::max();
    }

    // Dibujar el modelo
    for (int i = 0; i < model.nfaces(); i++) {
        std::vector<int> face = model.face(i);
        Vec3f world[3];
        Vec3f screen[3];
        Vec2f uvs[3];
        Vec3f norms[3];
        for (int j = 0; j < 3; j++) {
            world[j] = m2v(perspective * v2m(model.vert(face[j])));
            screen[j].x = (world[j].x + 1.0f) * width / 2.0f;
            screen[j].y = (world[j].y + 1.0f) * height / 2.0f;
            screen[j].z = world[j].z;
            uvs[j] = model.uv(i, j);
            norms[j] = model.norm(i, j);
        }
        triangle(screen, uvs, norms, model, image, light, zbuffer);
    }

    image.flip_vertically();
    image.write_png_file("images/output.png");
    return 0;
}