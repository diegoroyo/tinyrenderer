#include "geometry.h"
#include "pngimage/pngimage.h"
#include "pngimage/rgbcolor.h"

Matrix lookat(Vec3f eye, Vec3f center, Vec3f up);
Matrix getViewport(int x, int y, int w, int h, int d);
Matrix getProjection(float c);

struct IShader {
    virtual Vec3f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, RGBColor& color) = 0;
};

void triangle(Vec3f* pts, IShader& shader, PNGImage& image, float* zbuffer);