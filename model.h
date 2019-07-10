#pragma once

#include <vector>
#include "geometry.h"

// Modelos en formato wavefront .obj
// más info: https://en.wikipedia.org/wiki/Wavefront_.obj_file
// Solo lee los vertices y caras del modelo
class Model {
   private:
    std::vector<Vec3f> verts;
    std::vector<std::vector<int>> faces;

   public:
    Model(const char *filename);
    ~Model();
    int nverts();
    int nfaces();
    Vec3f vert(int i);
    std::vector<int> face(int idx);
};