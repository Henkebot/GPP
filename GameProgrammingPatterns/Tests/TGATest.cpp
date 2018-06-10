#include "../Other/TGAImage.h"

TGA_Color red = TGA_Color(255, 0, 0, 255);
TGA_Color white = TGA_Color(255, 255, 255, 255);

void line(int x0, int y0, int x1, int y1, TGA_Image& image, TGA_Color& color);

int main()
{
	bool Result = false;
	TGA_Image image(100, 100, TGA_Image::RGB);

	line(13, 20, 80, 40, image, white);
	line(20, 13, 40, 80, image, red);
	line(80, 40, 13, 20, image, red);

	image.flip_vertically();
	image.write_tga_file("output.tga");
	return 0;
}

void line(int x0, int y0, int x1, int y1, TGA_Image & image, TGA_Color & color)
{
	bool steep = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1))
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}

	if (x0 > x1)
	{
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	int dx = x1 - x0;
	int dy = y1 - y0;
	int derror2 = std::abs(dy) * 2;
	int error2 = 0;
	int y = y0;

	for(float x = x0; x<= x1; x++)
	{
		if (steep)
		{
			image.set(y, x, color);
		}
		else
		{
			image.set(x, y, color);
		}
		error2 += derror2;
		if (error2 > dx)
		{
			y += (y1 > y0) ? 1 : -1;
			error2 -= dx * 2;
		}
	}
}
