#include "TGAImage.h"
#include <iostream>

bool TGA_Image::load_rle_data(std::ifstream & in)
{
	unsigned long pixelcount = width * height;
	unsigned long currentpixel = 0;
	unsigned long currentbyte = 0;
	TGA_Color colorBuffer;
	do
	{
		unsigned char chunkheader = 0;
		chunkheader = in.get();
		if (!in.good())
		{
			std::cerr << "an error occured while reading the data\n";
			return false;
		}

		if (chunkheader < 128)
		{
			chunkheader++;
			for (int i = 0; i < chunkheader; i++)
			{
				in.read(reinterpret_cast<char*>(colorBuffer.raw), bytesPerPixel);
				if (!in.good())
				{
					std::cerr << "an error occured while reading the header\n";
					return false;
				}
				for (int t = 0; t < bytesPerPixel; t++)
				{
					data[currentbyte++] = colorBuffer.raw[t];
				}
				currentpixel++;
				if (currentpixel > pixelcount)
				{
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
		else
		{
			chunkheader -= 127;
			in.read(reinterpret_cast<char*>(colorBuffer.raw), bytesPerPixel);
			if (!in.good())
			{
				std::cerr << "an error occurerd whiel reading the header\n";
				return false;
			}
			for (int i = 0; i < chunkheader; i++)
			{
				for (int t = 0; t < bytesPerPixel; t++)
				{
					data[currentbyte++] = colorBuffer.raw[t];
				}
				currentpixel++;
				if (currentpixel > pixelcount)
				{
					std::cerr << "Too many pixels read.\n";
					return false;
				}
			}
		}
	} while (currentpixel < pixelcount);
	

	return true;
}

bool TGA_Image::unload_rle_data(std::ofstream & out)
{
	const unsigned char max_chunk_length = 128;
	unsigned long npixels = width* height;
	unsigned long curpix = 0;
	while(curpix < npixels)
	{ 
		unsigned long chunkstart = curpix*bytesPerPixel;
		unsigned long curbyte = curpix*bytesPerPixel;
		unsigned char run_length = 1;
		bool raw = true;

		while (curpix + run_length < npixels && run_length < max_chunk_length)
		{
			bool succ_eq = true;
			for (int t = 0; succ_eq && t < bytesPerPixel; t++)
			{
				succ_eq = (data[curbyte + t] == data[curbyte + t + bytesPerPixel]);
			}

			curbyte += bytesPerPixel;
			if (1 == run_length)
			{
				raw = !succ_eq;
			}
			if (raw && succ_eq)
			{
				run_length--;
				break;
			}
			if (!raw && !succ_eq)
			{
				break;
			}
			run_length++;
		}
		curpix += run_length;
		out.put(raw ? run_length - 1 : run_length + 127);
		if (!out.good())
		{
			std::cerr << "cant dump the tga file\n";
			return false;
		}
		out.write(reinterpret_cast<char*>(data + chunkstart), (raw ? run_length*bytesPerPixel : bytesPerPixel));
		if (!out.good())
		{
			std::cerr << "cant dump the tga file\n";
			return false;
		}
	}
	return true;
}

TGA_Image::TGA_Image() : data(NULL), width(0), height(0), bytesPerPixel(0)
{
}

TGA_Image::TGA_Image(int w_, int h_, int bpp_)
	: data(NULL), width(w_), height(h_), bytesPerPixel(bpp_)
{
	unsigned long nbytes = width * height * bytesPerPixel;
	data = new unsigned char[nbytes];
	memset(data, 0, nbytes);
}

TGA_Image::TGA_Image(const TGA_Image & img_)
{
}

TGA_Image::~TGA_Image()
{
}

bool TGA_Image::read_tga_file(const char * path_)
{
	delete[] data;
	data = NULL;
	std::ifstream in;
	in.open(path_, std::ios::binary);
	if(!in.is_open())
	{
		in.close();
		std::cerr << "Failed to open TGA Image.\n";
		return false;
	}

	TGA_Header header;
	in.read(reinterpret_cast<char*>(&header), sizeof(header));
	if(!in.good())
	{
		in.close();
		std::cerr << "An Error occured while reading the header.\n";
		return false;
	}

	width = header.width;
	height = header.height;
	bytesPerPixel = header.bitsperpixel >> 3;

	if(width <= 0 || height <= 0 || (bytesPerPixel != GREYSCALE && bytesPerPixel != RGB && bytesPerPixel != RGBA))
	{
		in.close();
		std::cerr << "Bad BytesPerPixel (or width/height) value\n";
		return false;
	}

	unsigned long nbytes = bytesPerPixel * width * height;
	data = new unsigned char[nbytes];

	if (3 == header.datatypecode || 2 == header.datatypecode)
	{
		in.read(reinterpret_cast<char*>(data), nbytes);
		if (!in.good())
		{
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	else if (10 == header.datatypecode || 11 == header.datatypecode)
	{
		if (!load_rle_data(in))
		{
			in.close();
			std::cerr << "An error occured while reading the data\n";
			return false;
		}
	}
	else
	{
		in.close();
		std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
		return false;
	}

	if (!(header.imagedescriptor & 0x20))
	{
		flip_vertically();
	}

	if (header.imagedescriptor & 0x10)
	{
		flip_horizontally();
	}

	std::cerr << width << "x" << height << "/" << bytesPerPixel * 8 << "\n";
	in.close();
	return true;
}

bool TGA_Image::write_tga_file(const char * path_, bool rle)
{
	unsigned char developer_area_ref[4] = { 0, 0, 0, 0 };
	unsigned char extension_area_ref[4] = { 0 ,0, 0, 0 };
	unsigned char footer[18] = { 'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0' };
	std::ofstream out;
	out.open(path_, std::ios::binary);
	if (!out.is_open())
	{
		std::cerr << "Can't open file " << path_ << "\n";
		out.close();
		return false;
	}

	TGA_Header header;
	memset(reinterpret_cast<void*>(&header), 0, sizeof(header));
	header.bitsperpixel = bytesPerPixel << 3;
	header.width = width;
	header.height = height;
	header.datatypecode = (bytesPerPixel == GREYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
	header.imagedescriptor = 0x20; 
	out.write(reinterpret_cast<char*>(&header), sizeof(header));
	if (!out.good())
	{
		out.close();
		std::cerr << "Can't dump the tga file.\n";
		return false;
	}
	if (!rle)
	{
		out.write(reinterpret_cast<char*>(data), width*height*bytesPerPixel);
		if (!out.good())
		{
			out.close();
			std::cerr << "Can't unload raw data\n";
			return false;
		}
	}
	else
	{
		if (!unload_rle_data(out))
		{
			out.close();
			std::cerr << "Can't unload rle data\n";
			return false;
		}
	}

	out.write(reinterpret_cast<char*>(developer_area_ref), sizeof(developer_area_ref));
	if (!out.good())
	{
		std::cerr << "Can't dump the tga file\n";
		out.close();
		return false;
	}

	out.write(reinterpret_cast<char*>(extension_area_ref), sizeof(extension_area_ref));
	if (!out.good())
	{
		std::cerr << "Can't dump the tga file\n";
		out.close();
		return false;
	}

	out.write(reinterpret_cast<char*>(footer), sizeof(footer));
	if (!out.good())
	{
		std::cerr << "Can't dump the tga file\n";
		out.close();
		return false;
	}
	out.close();

	return true;
}

bool TGA_Image::flip_horizontally()
{
	return false;
}

bool TGA_Image::flip_vertically()
{
	if (!data) return false;
	unsigned long bytes_per_line = width* bytesPerPixel;
	unsigned char* line = new unsigned char[bytes_per_line];
	int half = height >> 1;
	for (int j = 0; j < half; j++)
	{
		unsigned long l1 = j * bytes_per_line;
		unsigned long l2 = (height - 1 - j) * bytes_per_line;
		memmove(reinterpret_cast<void*>(line),		reinterpret_cast<void*>(data + l1),	bytes_per_line);
		memmove(reinterpret_cast<void*>(data + l1), reinterpret_cast<void*>(data + l2), bytes_per_line);
		memmove(reinterpret_cast<void*>(data + l2),	reinterpret_cast<void*>(line),		bytes_per_line);
	}
	delete[] line;
	return true;
}

bool TGA_Image::scale(int w, int h)
{
	return false;
}

TGA_Color TGA_Image::get(int x, int y)
{
	if (!data || x < 0 || y < 0 || x >= width || y >= height)
	{
		return TGA_Color();
	}
	return TGA_Color(data + (x + y * width) * bytesPerPixel, bytesPerPixel);
}

bool TGA_Image::set(int x, int y, TGA_Color c)
{
	if(!data || x < 0 || y < 0 || x>= width || y >= height)
	{
		return false;
	}
	memcpy(data + (x + y * width) * bytesPerPixel, c.raw, bytesPerPixel);
	return true;
}

int TGA_Image::get_width() const
{
	return width;
}

int TGA_Image::get_height() const
{
	return height;
}
