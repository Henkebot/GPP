#pragma once
#include <stdint.h>
#include <intsafe.h>
#include <glm.hpp>

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
	public:
		virtual glm::vec4 Vertex() = 0;
		virtual glm::vec4 Fragment() = 0;

	} SHADER, *LPSHADER;
	
	class RenderDevice
	{
	private:
		glm::mat4 m_ViewportMat;
		LPSHADER m_Shader;

		

	};





}

