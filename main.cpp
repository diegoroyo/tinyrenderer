#include <iostream>
#include <limits>
#include "model.h"
#include "our_gl.h"

static Model* model;
static Vec3f light(1, 1, 1);

static Matrix modelView, viewport, projection;

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

struct GouraudShader : public IShader {
    Vec3f varying_intensity;  // varying: escrito por vertex, leido por fragment

    Vec3f vertex(int iface, int nthvert) override {
        float intensity = light * model->norm(iface, nthvert);
        varying_intensity.raw[nthvert] = std::max(0.0f, intensity);
        Vec3f vert = model->vert(model->face(iface)[nthvert]);
        return m2v(viewport * projection * modelView * v2m(vert));
    }

    bool fragment(Vec3f bar, RGBColor& color) override {
        float intensity = varying_intensity * bar;
        color = RGBColor::White * intensity;
        return false;
    }
};

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <model_name>" << std::endl;
        return 1;
    }

    const int width = 800, height = 800;
    model = new Model(argv[1]);
    PNGImage image(width, height, RGBColor::Black);

    light.normalize();
    Vec3f camera(7.0f, 7.0f, 7.0f);
    Vec3f eye(-1.0f, -1.0f, -1.0f);
    Vec3f up(0.0f, 0.0f, 1.0f);

    modelView = lookat(camera, eye.normalize(), up);
    viewport = getViewport(0, 0, width, height, 255);
    projection = getProjection((camera - eye).norm());

    // Inicializar z-buffer a numeros negativos
    float zbuffer[height * width];
    for (int i = 0; i < height * width; i++) {
        zbuffer[i] = -1.0f * std::numeric_limits<float>::max();
    }

    // Dibujar el modelo
    GouraudShader shader;
    for (int i = 0; i < model->nfaces(); i++) {
        Vec3f screen[3];
        for (int j = 0; j < 3; j++) {
            screen[j] = shader.vertex(i, j);
        }
        triangle(screen, shader, image, zbuffer);
    }

    image.flip_vertically();
    image.write_png_file("images/output.png");
    return 0;
}