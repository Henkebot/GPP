#pragma once

#include <vector>
#include "Geometry.h"
#include "../Other/tgaimage.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<std::vector<Vec3i> > faces_; // attention, this Vec3i means vertex/uv/normal
	std::vector<Vec3f> norms_;
	std::vector<Vec2f> uv_;
	TGA_Image diffusemap_;
	void load_texture(std::string filename, const char *suffix, TGA_Image &img);
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int i);
	Vec2i uv(int iface, int nvert);
	TGA_Color diffuse(Vec2i uv);
	std::vector<int> face(int idx);
};