#pragma once

#include <cmath>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class t> 
struct Vec2 {
	union {
		struct { t u, v; };
		struct { t x, y; };
		t raw[2];
	};
	Vec2() : u(0), v(0) {}
	Vec2(t _u, t _v) : u(_u), v(_v) {}
	inline Vec2<t> operator +(const Vec2<t> &V) const { return Vec2<t>(u + V.u, v + V.v); }
	inline Vec2<t> operator -(const Vec2<t> &V) const { return Vec2<t>(u - V.u, v - V.v); }
	inline Vec2<t> operator *(float f)          const { return Vec2<t>(u*f, v*f); }
	template <class > friend std::ostream& operator<<(std::ostream& s, Vec2<t>& v);
	t&operator[](int index) { return raw[index]; }
};

template <class t> 
struct Vec3 {
	union {
		struct { t x, y, z; };
		struct { t ivert, iuv, inorm; };
		t raw[3];
	};
	Vec3() : x(0), y(0), z(0) {}
	Vec3(t _x, t _y, t _z) : x(_x), y(_y), z(_z) {}
	inline Vec3<t> operator ^(const Vec3<t> &v) const { return Vec3<t>(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }
	inline Vec3<t> operator +(const Vec3<t> &v) const { return Vec3<t>(x + v.x, y + v.y, z + v.z); }
	inline Vec3<t> operator -(const Vec3<t> &v) const { return Vec3<t>(x - v.x, y - v.y, z - v.z); }
	inline Vec3<t> operator *(float f)          const { return Vec3<t>(x*f, y*f, z*f); }
	inline t       operator *(const Vec3<t> &v) const { return x*v.x + y*v.y + z*v.z; }

	t norm() const { return std::sqrt(x*x + y*y + z*z); }
	Vec3<t> & normalize(t l = 1) { *this = (*this)*(l / norm()); return *this; }
	template <class > friend std::ostream& operator<<(std::ostream& s, Vec3<t>& v);
	t &operator[](int index) { if (index > 3) return raw[0]; 
							return raw[index]; }
	
};

typedef Vec2<float> Vec2f;
typedef Vec2<int>   Vec2i;
typedef Vec3<float> Vec3f;
typedef Vec3<int>   Vec3i;

template <class t> std::ostream& operator<<(std::ostream& s, Vec2<t>& v) {
	s << "(" << v.x << ", " << v.y << ")\n";
	return s;
}

template <class t> std::ostream& operator<<(std::ostream& s, Vec3<t>& v) {
	s << "(" << v.x << ", " << v.y << ", " << v.z << ")\n";
	return s;
}

const int DEFAULT_ALLOC = 4;

class Matrix {
	std::vector<std::vector<float> > m;
	int rows, cols;
public:
	Matrix(int r = DEFAULT_ALLOC, int c = DEFAULT_ALLOC);
	inline int nrows();
	inline int ncols();

	static Matrix identity(int dimensions);
	std::vector<float>& operator[](const int i);
	Matrix operator*(const Matrix& a);
	Vec3f operator*(Vec3f& a);
	Matrix transpose();
	Matrix inverse();

	friend std::ostream& operator<<(std::ostream& s, Matrix& m);
};

static Vec3f cross(Vec3f l, Vec3f r)
{
	return l^r;
}

static Vec3f 
barycentric(Vec3f *pts, Vec3i P) 
{
	Vec3f p(P.x, P.y, P.z);
	Vec3f a = pts[0];
	Vec3f b = pts[1];
	Vec3f c = pts[2];

	Vec3f v0 = b - a, v1 = c - a, v2 = p - a;
	float InvDen =  1.0f / ( v0.x * v1.y - v1.x * v0.y);
	float v = (v2.x * v1.y - v1.x * v2.y) * InvDen;
	float w = (v0.x * v2.y - v2.x * v0.y) * InvDen;
	float u = 1.0f - v - w;
	if(v >= 0 && u >= 0 && (u+v) <= 1)
		return Vec3f(u, v, w);
	return Vec3f(-1, 1, 1);
	////float s1 = pts[2].y - pts[0].y;
	////float s2 = pts[2].x - pts[0].x;
	////float s3 = pts[1].y - pts[0].y;
	////float s4 = P.y - pts[0].y;

	////float w1 = (pts[0].x * s1 + s4 * s2 - P.x * s1) / (s3 * s2 - (pts[1].x - pts[0].x) * s1);
	////float w2 = (s4 - w1 * s3) / s1;
	////if (w1 >= 0 && w2 >= 0 && (w1 + w2) <= 1)
	////{
	////	return Vec3f(w1, w2, P.z);
	////}
	////return Vec3f(-1, 1, 1);
	//Vec3f A = pts[0];
	//Vec3f B = pts[1];
	//Vec3f C = pts[2];
	//Vec3f s[2];
	//for (int i = 2; i--; ) {
	//	s[i][0] = C[i] - A[i];
	//	s[i][1] = B[i] - A[i];
	//	s[i][2] = A[i] - P[i];
	//}
	//Vec3f u = cross(s[0], s[1]);
	//if (std::abs(u[2])>1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
	//	return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	//return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
	//	
}
	

