#include "model.h"

#include <fstream>
#include <iostream>
#include <sstream>

void Model::load_texture(const char* filename) {
    std::string texturename(filename);
    size_t pos = texturename.find_last_of(".");
    if (pos == std::string::npos) {
        std::cerr << "Invalid obj filename" << std::endl;
    } else {
        texturename = texturename.substr(0, pos) + "_diffuse.png";
        diffuseMap.read_png_file(texturename.c_str());
        diffuseMap.flip_vertically();
    }
}

Model::Model(const char* filename) : verts(), faces(), uvs(), norms() {
    // Apertura
    std::ifstream is(filename);
    if (!is.is_open()) {
        std::cerr << "Can't open file " << filename << std::endl;
        is.close();
    }
    // Textura (almacenada en diffuseMap)
    load_texture(filename);
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
        } else if (line.compare(0, 3, "vt ") == 0) {
            iss >> ctrash >> ctrash;  // leer "vt"
            Vec2f v;
            // Leer textura y añadir
            for (int i = 0; i < 2; i++) iss >> v.raw[i];
            uvs.push_back(v);
        } else if (line.compare(0, 3, "vn ") == 0) {
            iss >> ctrash >> ctrash;  // leer "vn"
            Vec3f v;
            // Leer normal y añadir
            for (int i = 0; i < 3; i++) iss >> v.raw[i];
            norms.push_back(v);
        } else if (line.compare(0, 2, "f ") == 0) {
            iss >> ctrash;  // leer "f"
            std::vector<Vec3i> f;
            // Leer cara y añadir
            int idx, iduv, idnorm;
            // leer "idx/iduv/idnorm"
            // idx -> id de vertice (v)
            // iduv -> id de texture vertex (vt)
            // idnorm -> id de normal vertex (vn)
            while (iss >> idx >> ctrash >> iduv >> ctrash >> idnorm) {
                idx--;  // en formato .obj los indices empiezan en 1, no 0
                iduv--;
                idnorm--;
                f.push_back(Vec3i(idx, iduv, idnorm));
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
    std::vector<int> verts;
    for (Vec3i face : faces[idx]) verts.push_back(face.ivert);
    return verts;
}

Vec2f Model::uv(int iface, int nvert) {
    return uvs[faces[iface][nvert].iuv];
}

RGBColor Model::diffuse(Vec2f uv) {
    RGBColor color;
    diffuseMap.get_pixel((int)(uv.x * diffuseMap.width),
                         (int)(uv.y * diffuseMap.height), color);
    return color;
}