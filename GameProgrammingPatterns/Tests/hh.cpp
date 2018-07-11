#include <windows.h>
#include <stdint.h>
#include <fstream>
#include "../Timer/QPC.cpp"

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define RGBA(r,g,b,a) (a << 24 | r << 16 | g << 8 | b)
#define RGB(r,g,b) RGBA(r,g,b,255)
template <class T>
struct Vec
{
	T x;
	T y;

	T Dot(const Vec<T>& other) const
	{
		return x* other.x + y * other.y;
	}
	Vec<T> operator-(const Vec<T>& other) const
	{
		return { x - other.x, y - other.y};
	}
};

typedef Vec<int16> Vec2i;


struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	HBITMAP HBMP;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable int XOffset = 0;
global_variable int YOffset = 0;
global_variable int XMouse;
global_variable int YMouse;

internal void
Win32ClearBuffer(win32_offscreen_buffer* Buffer, int Color)
{
	memset(Buffer->Memory, 0, (Buffer->Width*Buffer->Height * 4));
}

inline internal void
Win32SetPixel(win32_offscreen_buffer* Buffer, int X, int Y, int color)
{
	
	int xTarget = (X + XOffset);
	int yTarget = (Y + YOffset);
	if (xTarget < 0 || xTarget >= Buffer->Width || yTarget < 0 || yTarget >= Buffer->Height) return;
	uint32 *pixel = (uint32*)Buffer->Memory + (xTarget + (yTarget * Buffer->Width));
	*pixel = color;
}


void line(int x0, int y0, int x1, int y1, win32_offscreen_buffer* Buffer, int color)
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

	for (float x = x0; x <= x1; x++)
	{
		if (steep)
		{
			Win32SetPixel(Buffer, y, x, color);
		}
		else
		{
			Win32SetPixel(Buffer, x, y, color);
		}
		error2 += derror2;
		if (error2 > dx)
		{
			y += (y1 > y0) ? 1 : -1;
			error2 -= dx * 2;
		}
	}
}

inline internal void
Plot4EllipsePoints(win32_offscreen_buffer* Buffer, Vec2i org, int X, int Y, int Color)
{
	Win32SetPixel(Buffer, org.x + X, org.y + Y, Color);
	Win32SetPixel(Buffer, org.x - X, org.y + Y, Color);
	Win32SetPixel(Buffer, org.x - X, org.y - Y, Color);
	Win32SetPixel(Buffer, org.x + X, org.y - Y, Color);
}

internal void
Win32DrawCircleQuick(win32_offscreen_buffer* Buffer, Vec2i vec, int radius, int color)
{
	
	int x = radius - 1;
	int y = 0;
	int dx = 1;
	int dy = 1;
	int err = dx - (radius << 1);

	while (x >= y)
	{
		Win32SetPixel(Buffer, vec.x + x, vec.y + y, color);
		Win32SetPixel(Buffer, vec.x + y, vec.y + x, color);
		Win32SetPixel(Buffer, vec.x - y, vec.y + x, color);
		Win32SetPixel(Buffer, vec.x - x, vec.y + y, color);
		Win32SetPixel(Buffer, vec.x - x, vec.y - y, color);
		Win32SetPixel(Buffer, vec.x - y, vec.y - x, color);
		Win32SetPixel(Buffer, vec.x + y, vec.y - x, color);
		Win32SetPixel(Buffer, vec.x + x, vec.y - y, color);

		if (err <= 0)
		{
			y++;
			err += dy;
			dy += 2;
		}

		if (err > 0)
		{
			x--;
			dx += 2;
			err += dx - (radius << 1);
		}
	}


}

internal void
Win32DrawCircleTest(win32_offscreen_buffer* Buffer, Vec2i Position, int Radius, int Color)
{
	int x = 0;
	int y = Radius;
	int p = 3 - 2 * Radius;
	int xc = Position.x;
	int yc = Position.y;

	while (y >= x) // only formulate 1/8 of circle
	{
		Win32SetPixel(Buffer, xc - x, yc - y,Color);//upper left left
		Win32SetPixel(Buffer, xc - y, yc - x,Color);//upper upper left
		Win32SetPixel(Buffer, xc + y, yc - x,Color);//upper upper right
		Win32SetPixel(Buffer, xc + x, yc - y,Color);//upper right right
		Win32SetPixel(Buffer, xc - x, yc + y,Color);//lower left left
		Win32SetPixel(Buffer, xc - y, yc + x,Color);//lower lower left
		Win32SetPixel(Buffer, xc + y, yc + x,Color);//lower lower right
		Win32SetPixel(Buffer, xc + x, yc + y,Color);//lower right right
		if (p < 0) p += 4 * x++ + 6;
		else p += 4 * (x++ - y--) + 10;
	}
}

internal void
Win32DrawCircleFill(win32_offscreen_buffer* Buffer, Vec2i Position, int Radius, int Color)
{
	// Taken from wikipedia
	int x = 0;
	int y = Radius;
	int p = 3 - 2 * Radius;
	int xc = Position.x;
	int yc = Position.y;

	auto drawline = [&](int sx, int ex, int ny)
	{
		for (int i = sx; i <= ex; i++)
			Win32SetPixel(Buffer, i, ny,Color);
	};

	while (y >= x)
	{
		// Modified to draw scan-lines instead of edges
		drawline(xc - x, xc + x, yc - y);
		drawline(xc - y, xc + y, yc - x);
		drawline(xc - x, xc + x, yc + y);
		drawline(xc - y, xc + y, yc + x);
		if (p < 0) p += 4 * x++ + 6;
		else p += 4 * (x++ - y--) + 10;
	}
}

internal void
Win32DrawCircle(win32_offscreen_buffer* Buffer, Vec2i vec, int radius, int color)
{
	int startX = vec.x - radius; int endX = startX + (radius << 1);
	int startY = vec.y - radius; int endY = startY + (radius << 1);
	int LookX = 0;
	int LookY = 0;
	int LookZ = 1;
	int radiusSqrt = radius * radius;
	
	Vec2i cur;


	for (cur.x = startX; cur.x < endX; cur.x++)
	{
		for (cur.y = startY; cur.y < endY; cur.y++)
		{
			Vec2i diff = cur - vec;

			int length = (diff.y*diff.y) + (diff.x* diff.x);

			if (length < radiusSqrt)
			{
				int c = color;
				
				float distance = 1 - (float(length) / radiusSqrt);
				int finalC = 128 * distance;
				c = RGB(finalC, finalC, finalC);			

				Win32SetPixel(Buffer, cur.x, cur.y, c);
			}
		}
	}

}

internal void
Win32DrawRect(win32_offscreen_buffer* Buffer, RECT rect, int color)
{
	int xPos = rect.left;
	int yPos = rect.top;

	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	for (int X = 0; X < width; ++X)
	{
		for (int Y = 0; Y < height; ++Y)
		{
			Win32SetPixel(Buffer, X + xPos, Y + yPos, color);
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

void wuline(int x0, int y0, int x1, int y1, win32_offscreen_buffer* Buffer, int Color)
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
		Win32SetPixel(Buffer, ypxl1, xpxl1, 0xffffff * rfpart(yend) * xgap);
		Win32SetPixel(Buffer, ypxl1 + 1, xpxl1, 0xffffff * fpart(yend) * xgap);
	}
	else
	{
		Win32SetPixel(Buffer, xpxl1, ypxl1, 0xffffff * rfpart(yend) * xgap);
		Win32SetPixel(Buffer, xpxl1, ypxl1 + 1, 0xffffff * fpart(yend) * xgap);
	}
	float intery = yend + gradient;

	xend = round(x1);
	yend = y1 + gradient * (xend - x1);
	xgap = fpart(x1 + 0.5f);
	float xpxl2 = xend;
	float ypxl2 = ipart(yend);

	if (steep)
	{
		Win32SetPixel(Buffer, ypxl2, xpxl2, 0xffffff * rfpart(yend) * xgap);
		Win32SetPixel(Buffer, ypxl2 + 1, xpxl2, 0xffffff * fpart(yend) * xgap);
	}
	else
	{
		Win32SetPixel(Buffer, xpxl2, ypxl2, 0xffffff * rfpart(yend) * xgap);
		Win32SetPixel(Buffer, xpxl2, ypxl2 + 1, 0xffffff * fpart(yend) * xgap);
	}

	if (steep)
	{
		for (int x = xpxl1 + 1; x < xpxl2 - 1; x++)
		{
			Win32SetPixel(Buffer, ipart(intery), x, 0xffffff * rfpart(intery));
			Win32SetPixel(Buffer, ipart(intery) + 1, x, 0xffffff * fpart(intery));
			intery = intery + gradient;
		}
	}
	else
	{
		for (int x = xpxl1 + 1; x < xpxl2 - 1; x++)
		{
			Win32SetPixel(Buffer, x, ipart(intery), 0xffffff * rfpart(intery));
			Win32SetPixel(Buffer, x, ipart(intery) + 1, 0xffffff * fpart(intery));
			intery = intery + gradient;
		}
	}
}

win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return (Result);
}



internal void
RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
	uint8 *Row = (uint8 *)Buffer.Memory;
	for (int Y = 0;
		Y < Buffer.Height;
		++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0;
			X < Buffer.Width;
			++X)
		{
			uint8 Blue = (X + XOffset);
			uint8 Green = (Y + YOffset);

			*Pixel++ = ((Green << 8) | Blue);
		}

		Row += Buffer.Pitch;
	}


}

internal void
Win32ResizeDIBSection(HDC DeviceContext, win32_offscreen_buffer *Buffer, int Width, int Height)
{

	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;

	int BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	//int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
	//Buffer->Memory = //VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	
	Buffer->HBMP = CreateDIBSection(DeviceContext, &Buffer->Info, DIB_RGB_COLORS, &Buffer->Memory, NULL,NULL);
	Buffer->Pitch = Width*BytesPerPixel;
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext,
	int WindowWidth, int WindowHeight,
	win32_offscreen_buffer Buffer)
{

	local_persist HDC hMemDC = CreateCompatibleDC(DeviceContext);
	local_persist HBITMAP hDCBitmap = (HBITMAP) SelectObject(hMemDC, Buffer.HBMP);
	BitBlt(DeviceContext, 0, 0, Buffer.Width, Buffer.Height, hMemDC, 0, 0, SRCCOPY);
	/*SelectObject(hMemDC, hDCBitmap);
	DeleteDC(hMemDC);*/
	//StretchDIBits(DeviceContext,
	//	/*
	//	X, Y, Width, Height,
	//	X, Y, Width, Height,
	//	*/
	//	0, 0, WindowWidth, WindowHeight,
	//	0, 0, Buffer.Width, Buffer.Height,
	//	Buffer.Memory,
	//	&Buffer.Info,
	//	DIB_RGB_COLORS, SRCCOPY);

}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
	case WM_DESTROY:
	{
		GlobalRunning = false;
	} break;

	case WM_CLOSE:
	{
		GlobalRunning = false;
	} break;

	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	} break;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	case WM_KEYDOWN:
	{
		int keyCode = WParam;
		bool keyDown = (LParam & (1 << 31)) == 0;
		bool isAltDown = (LParam & (1 << 29)) != 0;

		
		if(keyCode == 'W')
			YOffset -=5;
		if (keyCode == 'S')
			YOffset += 5;

		if (keyCode == 'D')
			XOffset += 5;
		if (keyCode == 'A')
			XOffset -= 5;
	
		
	} break;

	case WM_MOUSEMOVE:
	{
		POINTS p = MAKEPOINTS(LParam);
		XMouse = p.x;
		YMouse = p.y;
		
	} break;

	case WM_PAINT:
	{
		Win32ClearBuffer(&GlobalBackBuffer, 0);
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Window, &Paint);

		win32_window_dimension Dimension = Win32GetWindowDimension(Window);

		Win32DisplayBufferInWindow(DeviceContext,
			Dimension.Width,
			Dimension.Height,
			GlobalBackBuffer);
		EndPaint(Window, &Paint);
	} break;

	default:
	{
		Result = DefWindowProc(Window, Message, WParam, LParam);
	} break;
	}

	return(Result);
}


int CALLBACK
WinMain(HINSTANCE Instance, // a handle to our executable
	HINSTANCE PrevInstance,
	LPSTR CommandLine,
	int ShowCode)
{
	WNDCLASSA WindowClass = {};


	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	//WindowClass.hIcon = ;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClassA(&WindowClass))
	{
		HWND Window =
			CreateWindowExA(0,
				WindowClass.lpszClassName,
				"Lol",
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				0,
				0,
				Instance,
				0);
		if (Window)
		{
			HDC DeviceContext = GetDC(Window);

			Win32ResizeDIBSection(DeviceContext, &GlobalBackBuffer, 1280, 720);
			
			GlobalRunning = true;
			while (GlobalRunning)
			{
				ScopedTimer time("Main-Loop");
				MSG Message;
				while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}
				
				Win32ClearBuffer(&GlobalBackBuffer, 0);

				RECT rect = {};
				rect.right = 200;
				rect.bottom = 200;
				Win32DrawRect(&GlobalBackBuffer, rect, 0xff00ff);
		
		
				int xMiddle = 1280 >> 1;
				int yMiddle = 720 >> 1;
				
				int radius = sqrt(pow(xMiddle - XMouse, 2) + pow(yMiddle - YMouse, 2));
		
				
				//Win32DrawCircle(&GlobalBackBuffer, { (int16)xMiddle, (int16)yMiddle }, radius, RGB(0, 0, 255));
				Win32DrawCircleFill(&GlobalBackBuffer, { (int16)xMiddle, (int16)yMiddle }, radius, RGB(0, 0, 255));
				
				//wuline(xMiddle, yMiddle, XMouse, YMouse, &GlobalBackBuffer, 0xff00ff);
				//RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(DeviceContext,
					Dimension.Width,
					Dimension.Height,
					GlobalBackBuffer);

			}
		}
		else
		{
			// TODO(casey) Logging
		}
	}
	else
	{

		// TODO(casey): Logging
	}

	return (0);
}