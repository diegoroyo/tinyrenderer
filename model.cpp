#include "model.h"

#include <fstream>
#include <iostream>
#include <sstream>

Model::Model(const char *filename) : verts(), faces() {
    // Apertura
    std::ifstream is(filename);
    if (!is.is_open()) {
        std::cerr << "Can't open file " << filename << std::endl;
        is.close();
    }

    // Lectura
    std::string line;
    while (!is.eof()) {
        std::getline(is, line);
        std::istringstream iss(line.c_str());
        char ctrash;
        if (line.compare(0, 2, "v ") == 0) {
            iss >> ctrash;  // leer "v"
            Vec3f v;
            // Leer vertice y añadir
            for (int i = 0; i < 3; i++) iss >> v.raw[i];
            verts.push_back(v);
        } else if (line.compare(0, 2, "f ") == 0) {
            iss >> ctrash;  // leer "f"
            std::vector<int> f;
            // Leer cara y añadir
            int itrash, idx;
            // leer "idx/_/_", el primer numero hace referencia
            // a un vertice que forma la cara
            while (iss >> idx >> ctrash >> itrash >> ctrash >> itrash) {
                idx--;  // en formato .obj los indices empiezan en 1, no 0
                f.push_back(idx);
            }
            faces.push_back(f);
        }
    }
}

Model::~Model() {}

int Model::nverts() {
    return verts.size();
}

int Model::nfaces() {
    return faces.size();
}

Vec3f Model::vert(int i) {
    return verts[i];
}

std::vector<int> Model::face(int idx) {
    return faces[idx];
}