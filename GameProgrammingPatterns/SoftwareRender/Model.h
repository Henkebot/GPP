#pragma once

#include <vector>
#include "Geometry.h"

class Model
{
private:
	std::vector<Vec3f> m_vertices;
	std::vector<std::vector<int>> m_faces;
public:
	Model(const char* path_);
	virtual ~Model();
	inline int nvertes() const { return m_vertices.size(); }
	inline int nfaces() const { return m_faces.size(); }
	inline Vec3f vert(int i) const { return m_vertices[i]; }
	inline std::vector<int> face(int idx) const { return m_faces[idx]; }

};