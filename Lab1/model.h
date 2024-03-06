#pragma once

#include "geometry.h"
#include "tgaimage.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

struct Point {
    int vertexInd = 0;
    int textureInd = -1;
    Vec3f normal{ -1.1111f,-1,-1 };
};

struct Face {
    Point points[3];
    Vec3f normal;
};

class Part {
private:
public:
    std::vector<Face> faces;

    bool diffFlag = false;
    TGAImage diffusemap_;
    bool normFlag = false;
    TGAImage normalmap_;
    bool specFlag = false;
    TGAImage specularmap_;
    bool lightFlag = false;
    TGAImage lightmap_;

    Vec3f Ka;
    Vec3f Kd;
    Vec3f Ks;

    bool load_texture(std::string filename, const char* suffix, TGAImage& img);
    Vec3f diffuse(Vec2f uvf, Vec3f def);
    Vec3f light_map(Vec2f uvf);
    Vec3f normal(Vec2f uvf, Vec3f def);
    float specular(Vec2f uvf, float def);
};

class Model {
private:
    

    std::ifstream mtl_file;

    Part read_next_mtl(std::string dir);
    Vec3f calculateNormal(Face face);
public:
    std::vector<Part> parts;
    std::vector<Vec4f> vertexes;
    std::vector<Vec3f> normalies;
    std::vector<Vec3f> texturesCord;

    Model(std::string dir, std::string model_name);
    Model() {};
};