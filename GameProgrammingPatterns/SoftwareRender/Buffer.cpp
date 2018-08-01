#include "Buffer.h"


uint32_t* MY3D::RGBBuffer::_pixelData(int _x, int _y)
{
	int Width = GetWidth();
	int Height = GetHeight();

	if (_x < 0 || _x >= Width || _y < 0 || _y >= Height) 
		return nullptr;
	
	uint32_t *pixel = (uint32_t*)m_Memory + (_x + (_y* Width));
	return pixel;
}

MY3D::RGBBuffer::RGBBuffer()
{
}

MY3D::RGBBuffer::RGBBuffer(int _width, int _height)
{
	Resize(_width, _height);
}

void MY3D::RGBBuffer::Resize(int _width, int _height)
{
	if (m_Memory)
	{
		VirtualFree(m_Memory, 0, MEM_RELEASE);
	}

	int BytesPerPixel = 4;

	m_Info.bmiHeader.biSize = sizeof(m_Info.bmiHeader);
	m_Info.bmiHeader.biWidth =_width;
	m_Info.bmiHeader.biHeight = _height;
	m_Info.bmiHeader.biPlanes = 1;
	m_Info.bmiHeader.biBitCount = 32;
	m_Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (_width*_height)*BytesPerPixel;
	m_Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

void MY3D::RGBBuffer::Clear(int _value)
{
	int _x = GetWidth();
	int _y = GetHeight();
	for (int x = 0; x < _x;x++)
		for (int y = 0; y < _y;y++)
			SetPixel(x, y, _value);
		//memset(m_Memory, _value, GetWidth() * GetHeight() * 4);
}

void MY3D::RGBBuffer::SetPixel(glm::vec2 _coord, glm::vec4 _color)
{
	SetPixel(_coord.x, _coord.y, RGB(_color.x, _color.y, _color.z, _color.w));
}

void MY3D::RGBBuffer::SetPixel(int _x, int _y, int _value)
{

	uint32_t* Pixel = _pixelData(_x, _y);
	if(Pixel)
		*Pixel = _value;
}

int MY3D::RGBBuffer::GetPixel(int _x, int _y)
{
	uint32_t* Pixel = _pixelData(_x, _y);
	if(Pixel)
		return *Pixel;
	return -1;
}

int MY3D::RGBBuffer::GetWidth() const
{
	return m_Info.bmiHeader.biWidth;
}

int MY3D::RGBBuffer::GetHeight() const
{
	return m_Info.bmiHeader.biHeight;
}

LPVOID MY3D::RGBBuffer::GetBufferData()
{
	return m_Memory;
}

LPBITMAPINFO MY3D::RGBBuffer::GetBitmapInfo()
{
	return &m_Info;
}

float * MY3D::SimpleBuffer::_pixelData(int _x, int _y)
{
	int Width = GetWidth();
	int Height = GetHeight();

	if (_x < 0 || _x >= Width || _y < 0 || _y >= Height)
		return nullptr;

	float *pixel = (float*)m_Memory + (_x + (_y* Width));

	return pixel;
}

MY3D::SimpleBuffer::SimpleBuffer()
{
}

MY3D::SimpleBuffer::SimpleBuffer(int _width, int _height)
{
	Resize(_width, _height);
}

void MY3D::SimpleBuffer::Resize(int _width, int _height)
{
	if (m_Memory)
		delete m_Memory;
	m_Memory = new float[_width * _height];
}

void MY3D::SimpleBuffer::Clear(int _value)
{
	int _x = GetWidth();
	int _y = GetHeight();
	for (int x = 0; x < _x; x++)
		for (int y = 0; y < _y; y++)
			SetPixel(x, y, _value);
}

void MY3D::SimpleBuffer::SetPixel(int _x, int _y, int _value)
{
	float* pixel = _pixelData(_x, _y);
	if (pixel)
		*pixel = _value;
}

int MY3D::SimpleBuffer::GetPixel(int _x, int _y)
{
	float* pixel = _pixelData(_x, _y);
	if (pixel)
		return *pixel;
	return -1;
}

int MY3D::SimpleBuffer::GetWidth() const
{
	return m_Width;
}

int MY3D::SimpleBuffer::GetHeight() const
{
	return m_Height;
}

LPVOID MY3D::SimpleBuffer::GetBufferData()
{
	return LPVOID(m_Memory);
}
