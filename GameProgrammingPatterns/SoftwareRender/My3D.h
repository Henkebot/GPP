#pragma once
#include <stdint.h>
#include <intsafe.h>
#include "Buffer.h"
#include <glm.hpp>
#include "Model.h"
#include <unordered_map>

// ClearBits
#define COLOR_BUFFER 1 << 0
#define DEPTH_BUFFER 1 << 1

namespace MY3D
{

	typedef struct Viewport
	{
		int16_t X;
		int16_t Y;
		int16_t Width;
		int16_t Height;
		int16_t Near;
		int16_t Far;
	} VIEWPORT, *LPVIEWPORT;

	glm::mat4 SetViewport(VIEWPORT _vp);

	typedef class IShader
	{
	protected:
		std::unordered_map<const char*, glm::mat4> m_mat4;
	public:
		Model* m_data;

		virtual glm::vec4 Vertex(int vertexID) = 0;
		virtual glm::vec4 Fragment(int faceID, glm::vec3 inter) = 0;
		void setUniform4x4f(const char* _name, glm::mat4 _mat);

	} SHADER, *LPSHADER;
	
	class RenderDevice
	{
	private:
		glm::mat4 m_ViewportMat;
		
		// Currently bound shader
		LPSHADER m_Shader;
		
		RGBBuffer m_frontBuffer;
		RGBBuffer m_zBuffer;

		struct InterPixel
		{
			glm::ivec2 pixelCoord;
			glm::vec3 interpolation;
		};

	public:
		void Clear(int _buffer);
		void SetViewport(VIEWPORT _vp);

		void SetBufferSize(int _w, int _h);
		glm::vec2 GetBufferSize() const;
		void SetShader(LPSHADER _shader);

		void Render();
		LPRGBBUFFER GetFrontBuffer();
	private:
		inline void _Triangle(int faceID, glm::vec4 *pts);
		

	};





}

