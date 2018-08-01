#pragma once

#include <Windows.h>
#include <stdint.h>
#include <glm.hpp>

namespace MY3D
{

	class Buffer 
	{
	public:
		virtual int GetWidth() const = 0;
		virtual int GetHeight() const = 0;
		virtual void Resize(int _width, int _height) = 0;
		virtual void Clear(int _value = 0) = 0;

		virtual void SetPixel(int _x, int _y, int _value) = 0;
		virtual int GetPixel(int _x, int _y) = 0;
		virtual LPVOID GetBufferData() = 0;

		
	};

	class SimpleBuffer : public Buffer
	{
	private:
		float* m_Memory;
		int m_Width;
		int m_Height;
		float* _pixelData(int _x, int _y);
	public:
		SimpleBuffer();
		SimpleBuffer(int _width, int _height);

		void Resize(int _width, int _height);
		void Clear(int _value = 0);
		void SetPixel(int _x, int _y, int _value);
		int GetPixel(int _x, int _y);

		int GetWidth() const;
		int GetHeight() const;

		LPVOID GetBufferData();

	};

	typedef class RGBBuffer : public Buffer
	{
	private:
		BITMAPINFO	m_Info;
		LPVOID		m_Memory;

		uint32_t* _pixelData(int _x, int _y);
	public:
		RGBBuffer();
		RGBBuffer(int _width, int _height);

		void Resize(int _width, int _height);
		void Clear(int _value = 0);
		void SetPixel(glm::vec2 _coord, glm::vec4 _color);
		void SetPixel(int _x, int _y, int _value);
		int GetPixel(int _x, int _y);

		inline int GetWidth() const;
		inline int GetHeight() const;

		LPVOID GetBufferData();

		// RGB Specific
		LPBITMAPINFO GetBitmapInfo();

	} RGBBUFFER, *LPRGBBUFFER;



}
