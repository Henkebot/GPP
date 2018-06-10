#include "Model.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "Model.h"

Model::Model(const char * path_)
{
	std::ifstream in;
	in.open(path_, std::ifstream::in);
	if (in.fail()) return;
	std::string line;
	while (!in.eof()) {
		std::getline(in, line);
		std::istringstream iss(line.c_str());
		char trash;
		if (!line.compare(0, 2, "v ")) {
			iss >> trash;
			Vec3f v;
			for (int i = 0; i<3; i++) iss >> v.raw[i];
			m_vertices.push_back(v);
		}
		else if (!line.compare(0, 2, "f ")) {
			std::vector<int> f;
			int itrash, idx;
			iss >> trash;
			while (iss >> idx >> trash >> itrash >> trash >> itrash) {
				idx--; // in wavefront obj all indices start at 1, not zero
				f.push_back(idx);
			}
			m_faces.push_back(f);
		}
	}
	std::cerr << "# v# " << m_vertices.size() << " f# " << m_faces.size() << std::endl;
}

Model::~Model()
{
}
