#include "My3D.h"


glm::mat4 MY3D::SetViewport(VIEWPORT _vp)
{
	float w = _vp.Width;
	float h = _vp.Height;
	float n = _vp.Near;
	float f = _vp.Far;
	float ww = w * 0.5f;
	float hh = h * 0.5f;
	
	glm::mat4 vp(1);
	vp[0].x = ww;		/*0					0*/							vp[0].w= ww + _vp.X;
	/*0*/				vp[1].y = hh;		/*0*/						vp[1].w= hh + _vp.Y;
	/*0					0*/					vp[2].z = (n - f) * 0.5f;	vp[2].w = ((n + f) * 0.5f);

	return vp;
}

void MY3D::RenderDevice::Clear(int _buffer)
{
	if (_buffer & COLOR_BUFFER)
		m_frontBuffer.Clear(0);
	if (_buffer & DEPTH_BUFFER)
		m_zBuffer.Clear(-INT_MAX);

}

void MY3D::RenderDevice::SetViewport(VIEWPORT _vp)
{
	float w = _vp.Width;
	float h = _vp.Height;
	float n = _vp.Near;
	float f = _vp.Far;
	float ww = w * 0.5f;
	float hh = h * 0.5f;

	glm::mat4 vp(1);
	vp[0].x = ww;		/*0					0*/							vp[0].w = ww + _vp.X;
	/*0*/				vp[1].y = hh;		/*0*/						vp[1].w = hh + _vp.Y;
	/*0					0*/					vp[2].z = (n - f) * 0.5f;	vp[2].w = ((n + f) * 0.5f);

	m_ViewportMat = vp;
}

void MY3D::RenderDevice::SetBufferSize(int _w, int _h)
{
	m_frontBuffer.Resize(_w,_h);
	m_zBuffer.Resize(_w, _h);
}

glm::vec2 MY3D::RenderDevice::GetBufferSize() const
{
	return glm::vec2(m_frontBuffer.GetWidth(), m_frontBuffer.GetHeight());
}

void MY3D::RenderDevice::SetShader(LPSHADER _shader)
{
	m_Shader = _shader;
}

void MY3D::RenderDevice::Render()
{
	for (int i = m_Shader->m_data->nfaces(); i--;)
	{
		std::vector<int> face = m_Shader->m_data->face(i);
		// For every triangle we run the vertex shader 3 times
		glm::vec4 points[3];
		for (int j = 3; j--;)
		{
			points[j] = m_Shader->Vertex(face[j]);
		}

		//Backface culling, Projected points
		float ax = points[0].x - points[1].x;
		float ay = points[0].y - points[1].y;
		float bx = points[0].x - points[2].x;
		float by = points[0].y - points[2].y;
		float cz = ax * by - ay * bx;

		if (cz < 0) // This triangle is frontfacing!
		{
			// Now when we got the points we can send them to the rasterizer
			_Triangle(i, points);
		}

	}
}

MY3D::LPRGBBUFFER MY3D::RenderDevice::GetFrontBuffer()
{
	return &m_frontBuffer;
}

static glm::vec3 inline
barycentric(glm::vec2 *pts, glm::vec2 P)
{
	glm::vec2 a = pts[0];
	glm::vec2 b = pts[1];
	glm::vec2 c = pts[2];

	glm::vec2 v0 = b - a, v1 = c - a, v2 = P - a;
	float InvDen = 1.0f / (v0.x * v1.y - v1.x * v0.y);
	float v = (v2.x * v1.y - v1.x * v2.y) * InvDen;
	float w = (v0.x * v2.y - v2.x * v0.y) * InvDen;
	float u = 1.0f - v - w;
	if (v >= 0 && u >= 0 && (u + v) <= 1)
		return glm::vec3(u, v, w);
	return glm::vec3(-1, 1, 1);
}


void MY3D::RenderDevice::_Triangle(int faceID, glm::vec4 * pts)
{
	glm::vec4 pts1[3];

	pts1[0] = pts[0] * m_ViewportMat;
	pts1[1] = pts[1] * m_ViewportMat;
	pts1[2] = pts[2] * m_ViewportMat;

	glm::vec2 pts2[3];
	glm::vec3 ptsForDepth[3];
	// Pts2 are the projected points
	pts2[0] = ptsForDepth[0] = (pts1[0] / pts1[0].w);
	pts2[1] = ptsForDepth[1] = (pts1[1] / pts1[1].w);
	pts2[2] = ptsForDepth[2] = (pts1[2] / pts1[2].w);


	int Width = m_frontBuffer.GetWidth();
	int Height = m_frontBuffer.GetHeight();
	glm::vec2 bboxmin(Width, Height);
	glm::vec2 bboxmax(-Width, -Height);
	glm::vec2 clamp(Width, Height);

	for (int i = 3; i--;) {
		for (int j = 2; j--;) {
			bboxmin[j] = glm::max(0.f, glm::min(bboxmin[j], pts2[i][j]));
			bboxmax[j] = glm::min(clamp[j], glm::max(bboxmax[j], pts2[i][j]));
		}
	}
	glm::ivec2 P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {

			glm::vec3 bc_screen = barycentric(pts2, P);

			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
				continue;

			glm::vec3 bc_clip = glm::vec3(bc_screen.x / pts1[0].w, bc_screen.y / pts1[1].w, bc_screen.z / pts1[2].w);
			bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);

			float currentDepth = m_zBuffer.GetPixel(P.x, P.y);
			if (currentDepth == -1)
				continue;


			float fragDepth = 0;

			fragDepth += ptsForDepth[0].z * bc_clip[0];
			fragDepth += ptsForDepth[1].z * bc_clip[1];
			fragDepth += ptsForDepth[2].z * bc_clip[2];

			if (currentDepth < fragDepth)
			{
				m_zBuffer.SetPixel(P.x, P.y, fragDepth);

				glm::vec4 color = m_Shader->Fragment(faceID, bc_clip);
				m_frontBuffer.SetPixel(P, color);

			}

		}
	}
	
}



void MY3D::IShader::setUniform4x4f(const char * _name, glm::mat4 _mat)
{
	m_mat4[_name] = _mat;
}
