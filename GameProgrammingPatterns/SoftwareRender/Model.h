#pragma once

#include <vector>
#include <glm.hpp>
#include "../Other/tgaimage.h"

class Model {
private:
	std::vector<glm::vec3> verts_;
	std::vector<std::vector<glm::ivec3> > faces_; // attention, this Vec3i means vertex/uv/normal
	std::vector<glm::vec3> norms_;
	std::vector<glm::vec3> uv_;
	TGA_Image diffusemap_;
	void load_texture(std::string filename, const char *suffix, TGA_Image &img);
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	glm::vec3 vert(int i);
	glm::vec3 norm(int iface, int nvert);
	glm::vec2 uv(int iface, int nvert);
	TGA_Color diffuse(glm::ivec2 uv);
	std::vector<int> face(int idx);
};