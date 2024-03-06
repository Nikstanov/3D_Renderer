#include "model.h"

bool Part::load_texture(std::string filename, const char* suffix, TGAImage& img)
{
    std::string texfile(filename);
    /*
    size_t dot = texfile.find_last_of(".");
    if (dot != std::string::npos) {
        texfile = texfile.substr(0, dot) + std::string(suffix);
        std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
        img.flip_vertically();
    }
    */
    if (!img.read_tga_file(texfile.c_str())) { return false; }
    img.flip_vertically();
    return true;
}

Vec3f Part::diffuse(Vec2f uvf, Vec3f def)
{
    if (!diffFlag) { return def; }
    Vec2i uv(uvf[0] * diffusemap_.get_width(), uvf[1] * diffusemap_.get_height());
    TGAColor color = diffusemap_.get(uv[0], uv[1]);
    return Vec3f(color[0] / 255.f, color[1] / 255.f, color[2] / 255.f);
}

Vec3f Part::light_map(Vec2f uvf)
{
    if (!lightFlag) { return Vec3f(.0f,.0f,.0f); }
    Vec2i uv(uvf[0] * lightmap_.get_width(), uvf[1] * lightmap_.get_height());
    TGAColor color = lightmap_.get(uv[0], uv[1]);
    return Vec3f(color[0] / 255.f, color[1] / 255.f, color[2] / 255.f);
}

Vec3f Part::normal(Vec2f uvf, Vec3f def)
{
    if (!normFlag) {return def;}
    Vec2i uv(uvf[0] * normalmap_.get_width(), uvf[1] * normalmap_.get_height());
    TGAColor c = normalmap_.get(uv[0], uv[1]);
    Vec3f res;
    for (int i = 0; i < 3; i++)
        res[2 - i] = (float)c[i] / 255.f * 2.f - 1.f;
    return res;
}

float Part::specular(Vec2f uvf, float def)
{
    if (!specFlag) { return def; }
    Vec2i uv(uvf[0] * specularmap_.get_width(), uvf[1] * specularmap_.get_height());
    return specularmap_.get(uv[0], uv[1])[0] / 1.f;
}

Part Model::read_next_mtl(std::string dir)
{
    Part part{};
    if (mtl_file.is_open()) {

        std::string line;
        while (std::getline(mtl_file, line)) {
            if (line == "") {
                break;
            }
            if (line.substr(0, 7) == "map_Kd ") {
                if (part.load_texture(dir + "/" + line.substr(7), "", part.diffusemap_)) { part.diffFlag = true; }
            }
            if (line.substr(0, 7) == "map_Ke ") {
                if(part.load_texture(dir + "/" + line.substr(7), "", part.lightmap_)) { part.lightFlag = true; }
            }
            if (line.substr(0, 5) == "norm ") {
                if(part.load_texture(dir + "/" + line.substr(5), "", part.normalmap_)) { part.normFlag = true; }
            }
            if (line.substr(0, 9) == "map_MRAO ") {
                if(part.load_texture(dir + "/" + line.substr(9), "", part.specularmap_)) { part.specFlag = true; }
            }
        }
    }
    return part;
}

Vec3f Model::calculateNormal(Face face)
{
    return cross((Vec3f)(vertexes[face.points[2].vertexInd] - vertexes[face.points[0].vertexInd]), (Vec3f)(vertexes[face.points[1].vertexInd] - vertexes[face.points[0].vertexInd])).normalize();
}



Model::Model(std::string dir, std::string model_name)
{
    bool firstObj = true;
    std::ifstream in;
    in.open(dir + "/" + model_name + ".obj");
    if (in.is_open()) {
        Part part {};
        std::string line;
        while (std::getline(in, line)) {
            if (line.substr(0, 7) == "mtllib ") {
                std::string test = line.substr(7);
                mtl_file.open(dir + "/" + line.substr(7));
                if (!mtl_file.is_open()) {
                    mtl_file.open(dir + "/" + model_name + ".mtl");
                }
            }
            if (line.substr(0, 2) == "o ") {
                if (!firstObj) {
                    parts.push_back(part);
                }
                part = read_next_mtl(dir);
            }
            if (line.substr(0, 2) == "v ") {
                Vec4f vert;
                vert[3] = 1.0f;
                sscanf_s(line.c_str(), "v %f %f %f", &vert[0], &vert[1], &vert[2]);
                vertexes.push_back(vert);
            }
            if (line.substr(0, 3) == "vn ") {
                Vec3f normal;
                sscanf_s(line.c_str(), "vn %f %f %f", &normal.x, &normal.y, &normal.z);
                normalies.push_back(normal.normalize());
            }
            if (line.substr(0, 3) == "vt ") {
                Vec3f texture;
                sscanf_s(line.c_str(), "vt %f %f %f", &texture.x, &texture.y, &texture.z);
                texturesCord.push_back(texture);
            }
            if (line.substr(0, 2) == "f ") {
                Face face;
                std::istringstream v(line.substr(2));
                std::string token;
                int ind = 0;
                while (v >> token) {
                    Point p;
                    int normalInd = -1;
                    sscanf_s(token.c_str(), "%i/%i/%i", &p.vertexInd, &p.textureInd, &normalInd);
                    p.vertexInd = p.vertexInd - 1;
                    p.textureInd = p.textureInd - 1;
                    if (normalInd > 0) p.normal = normalies[normalInd - 1];
                    face.points[ind] = p;
                    ind++;
                }
                part.faces.push_back(face);
            }
        }
        parts.push_back(part);
        in.close();
    }


    for (int ind = 0; ind < parts.size(); ind++) {
        std::map<int, std::vector<Face*>> map;
        for (size_t i = 0; i < parts[ind].faces.size(); i++) {
            Face face = parts[ind].faces[i];
            parts[ind].faces[i].normal = calculateNormal(parts[ind].faces[i]);
            for (size_t j = 0; j < 3; j++) {
                map[parts[ind].faces[i].points[j].vertexInd].push_back(&parts[ind].faces[i]);
            }
        }


        for (size_t i = 0; i < parts[ind].faces.size(); i++) {
            for (size_t j = 0; j < 3; j++) {
                if (parts[ind].faces[i].points[j].normal.x == -1.1111f) {
                    std::vector<Face*> a = map[parts[ind].faces[i].points[j].vertexInd];
                    Vec3f norm{};
                    for (size_t k = 0; k < a.size(); k++) {
                        norm = norm + a[k]->normal;
                    }
                    norm = (norm / a.size()).normalize();
                    parts[ind].faces[i].points[j].normal = norm;
                }
            }
        }
    }

    
}
