/*************************************
Source https://github.com/ssloy/tinyrenderer/tree/909fe20934ba5334144d2c748805690a1fa4c89f
**************************************/

#pragma once
#include <fstream>

#pragma pack(push, 1)
struct TGA_Header
{
	char idlength;
	char colormaptype;
	char datatypecode;
	short colormaporigin;
	short colormaplength;
	char colormapdepth;
	short x_origin;
	short y_origin;
	short width;
	short height;
	char bitsperpixel;
	char imagedescriptor;
};
#pragma pack(pop)

struct TGA_Color
{
	union
	{
		struct {
			unsigned char b, g, r, a;
		};
		unsigned char raw[4];
		unsigned int val;
	};
	int bytesPerPixel;

	TGA_Color() 
		: val(0), bytesPerPixel(1)
	{}

	TGA_Color(unsigned char R, unsigned char G, unsigned char B, unsigned char A) 
		:	b(B), g(G), r(R), a(A), bytesPerPixel(4) 
	{}

	TGA_Color(const unsigned char* p_, int bpp_)
		: val(0), bytesPerPixel(bpp_)
	{
		for (int i = 0; i < bytesPerPixel; i++)
		{
			raw[i] = p_[i];
		}
	}

	TGA_Color operator*(float val) const
	{
		return TGA_Color(r * val, g* val, b * val, a);
	}

};

class TGA_Image
{
private:
	unsigned char* data;
	int width;
	int height;
	int bytesPerPixel;

	bool load_rle_data(std::ifstream& in);
	bool unload_rle_data(std::ofstream& out);
public:
	enum Format
	{
		GREYSCALE = 1, RGB = 3, RGBA = 4
	};

	TGA_Image();
	TGA_Image(int w_, int h_, int bpp_);
	TGA_Image(const TGA_Image& img_);

	virtual ~TGA_Image();

	bool read_tga_file(const char* path_);
	bool write_tga_file(const char* path_, bool rle = true);
	bool flip_horizontally();
	bool flip_vertically();
	bool scale(int w, int h);
	TGA_Color get(int x, int y);
	bool set(int x, int y, TGA_Color c);


};



