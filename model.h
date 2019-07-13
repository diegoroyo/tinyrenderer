#pragma once

#include <vector>
#include "geometry.h"
#include "pngimage/pngimage.h"
#include "pngimage/rgbcolor.h"

// Modelos en formato wavefront .obj
// más info: https://en.wikipedia.org/wiki/Wavefront_.obj_file
// Solo lee los vertices y caras del modelo
class Model {
   private:
    std::vector<Vec3f> verts;
    std::vector<std::vector<Vec3i>> faces;
    std::vector<Vec2f> uvs;
    std::vector<Vec3f> norms;
    PNGImage diffuseMap;

    void load_texture(const char *filename);

   public:
    Model(const char *filename);
    ~Model();
    int nverts();
    int nfaces();
    Vec3f vert(int i);
    std::vector<int> face(int idx);
    Vec2f uv(int iface, int nvert);
    RGBColor diffuse(Vec2f uv);
};