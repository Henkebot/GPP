#include "../Other/TGAImage.h"

TGA_Color red = TGA_Color(255, 0, 0, 255);
TGA_Color white = TGA_Color(255, 255, 255, 255);

void line(int x0, int y0, int x1, int y1, TGA_Image& image, TGA_Color& color);
void wuline(int x0, int y0, int x1, int y1, TGA_Image& image, TGA_Color& color);

int main()
{
	bool Result = false;
	TGA_Image image(100, 100, TGA_Image::RGB);

	wuline(13, 20, 80, 40, image, white);
	wuline(20, 13, 40, 80, image, red);
	wuline(80, 40, 13, 20, image, red);

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
namespace L
{
	float ipart(float x)
	{
		return floor(x);
	}

	float round(float x)
	{
		return ipart(x + 0.5f);
	}

	float fpart(float x)
	{
		return x - floor(x);
	}

	float rfpart(float x)
	{
		return 1 - fpart(x);
	}
}


void wuline(int x0, int y0, int x1, int y1, TGA_Image & image, TGA_Color & color)
{
	using namespace L;
	bool steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep)
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
	}

	if (x0 > x1)
	{
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	float dx = x1 - x0;
	float dy = y1 - y0;
	float gradient;
	if (dx == 0)
	{
		gradient = 1.0f;
	}
	else
	{
		gradient = dy / dx;
	}

	float xend = L::round(x0);
	float yend = y0 + gradient * (xend - x0);
	float xgap = rfpart(x0 + 0.5f);
	float xpxl1 = xend;
	float ypxl1 = ipart(yend);
	if (steep)
	{
		image.set(ypxl1		, xpxl1,	color * rfpart(yend) * xgap);
		image.set(ypxl1 + 1	, xpxl1,	color * fpart(yend) * xgap);
	}
	else
	{
		image.set(xpxl1,	 ypxl1, color * rfpart(yend) * xgap);
		image.set(xpxl1, ypxl1 + 1, color * fpart(yend) * xgap);
	}
	float intery = yend + gradient;

	xend = round(x1);
	yend = y1 + gradient * (xend - x1);
	xgap = fpart(x1 + 0.5f);
	float xpxl2 = xend;
	float ypxl2 = ipart(yend);

	if (steep)
	{
		image.set(ypxl2, xpxl2, color* rfpart(yend) * xgap);
		image.set(ypxl2 + 1, xpxl2, color * fpart(yend) * xgap);
	}
	else
	{
		image.set(xpxl2, ypxl2, color * rfpart(yend) * xgap);
		image.set(xpxl2, ypxl2 + 1, color * fpart(yend) * xgap);
	}

	if (steep)
	{
		for (int x = xpxl1 + 1; x < xpxl2 - 1; x++)
		{
			image.set(ipart(intery), x, color * rfpart(intery));
			image.set(ipart(intery) + 1, x, color*fpart(intery));
			intery = intery + gradient;
		}
	}
	else
	{
		for (int x = xpxl1 + 1; x < xpxl2 - 1; x++)
		{
			image.set(x, ipart(intery), color * rfpart(intery));
			image.set(x, ipart(intery) + 1, color * fpart(intery));
			intery = intery + gradient;
		}
	}
}
