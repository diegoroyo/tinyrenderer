#include "our_gl.h"
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

void triangle(Vec3f* t, IShader& shader, PNGImage& image, float* zbuffer) {
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
                    // Calcular el color (pixel x-y) de la textura
                    // y multiplicarlo por la intensidad (luz)
                    RGBColor color;
                    bool discard = shader.fragment(bc_coords, color);
                    if (!discard) {
                        zbuffer[x + y * image.width] = z;
                        image.set_pixel(x, y, color);
                    }
                }
            }
        }
    }
}

// devuelve la matriz modelview (coordenadas del modelo
// a coordenadas de la cámara)
Matrix lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye - center).normalize();
    Vec3f x = (up ^ z).normalize();
    Vec3f y = (z ^ x).normalize();
    Matrix Minv = Matrix::identity(4);
    Matrix Tr = Matrix::identity(4);
    for (int i = 0; i < 3; i++) {
        Minv[0][i] = x.raw[i];
        Minv[1][i] = y.raw[i];
        Minv[2][i] = z.raw[i];
        Tr[i][3] = -center.raw[i];
    }
    return Minv * Tr;
}

// mapeo del cubo [-1, 1] * [-1, 1] * [-1, 1]
// al cubo [x, x + w] * [y, y + h] * [0, d]
Matrix getViewport(int x, int y, int w, int h, int d) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x + w / 2.0f;
    m[1][3] = y + h / 2.0f;
    m[2][3] = d / 2.0f;

    m[0][0] = w / 2.0f;
    m[1][1] = h / 2.0f;
    m[2][2] = d / 2.0f;
    return m;
}

// Matriz de proyección en perspectiva
// c: distancia entre la cámara y el centro de la escena
Matrix getProjection(float c) {
    Matrix m = Matrix::identity(4);
    m[3][2] = -1.0f / c;
    return m;
}