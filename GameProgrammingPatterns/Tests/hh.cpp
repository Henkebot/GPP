#include <windows.h>
#include <stdint.h>
#include <fstream>
#include "../Timer/QPC.cpp"
#include "../SoftwareRender/Model.h"
#include <limits>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm.hpp>
#include <gtx\transform.hpp>
#include <gtc/matrix_transform.hpp>

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
global_variable float XOffset = 0;
global_variable float YOffset = 0;
global_variable int XMouse;
global_variable int YMouse;
global_variable float W;
global_variable float S;
global_variable Model* GlobalModel;

global_variable float* GlobalZBuffer;



internal void
Win32ClearBuffer(win32_offscreen_buffer* Buffer, int Color)
{
	memset(Buffer->Memory, Color, Buffer->Height * Buffer->Width * 4);

}

inline internal void
Win32SetPixel(win32_offscreen_buffer* Buffer, int X, int Y, int color)
{
	
	int xTarget = (X );
	int yTarget = (Y );
	if (xTarget < 0 || xTarget >= Buffer->Width || yTarget < 0 || yTarget >= Buffer->Height) return;
	uint32 *pixel = (uint32*)Buffer->Memory + (xTarget + (yTarget * Buffer->Width));
	*pixel = color;
}

inline internal
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

void triangle(Vec3f *pts, win32_offscreen_buffer* Buffer, int color) {
	
	

	Vec2f bboxmin(Buffer->Width, Buffer->Height);
	Vec2f bboxmax(0,0);
	Vec2f clamp(Buffer->Width- 1, Buffer->Height- 1);
	
	for (int i = 3; i--;) {
		for (int j = 2; j--;) {
			bboxmin[j] = glm::max(0.f,		glm::min(bboxmin[j], pts[i][j]));
			bboxmax[j] = glm::min(clamp[j],	glm::max(bboxmax[j], pts[i][j]));
		}
	}
	Vec3i P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {

			Vec3f bc_screen = barycentric(pts, P);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;

			P.z = 0;
			for (int i = 3; i--;)
			{
				P.z += pts[i].z * bc_screen[i];
			}

			int zBufferIndex = P.x + P.y * Buffer->Width;

			float currentValue = GlobalZBuffer[zBufferIndex];
			if (currentValue > P.z)
			{
				GlobalZBuffer[zBufferIndex] = P.z;
				
				//Win32SetPixel(Buffer, P.x, P.y, RGB((int)(bc_screen.x *255.f), (int)(bc_screen.y*255.f), (int)(bc_screen.z*255.f)));
				//Win32SetPixel(Buffer, P.x, P.y, RGB((int)(P.z), (int)(P.z), (int)(P.z)));
				Win32SetPixel(Buffer, P.x, P.y, color);

			}
			//Win32SetPixel(Buffer, P.x, P.y, RGB((int)(P.z), (int)(P.z), (int)(P.z)));
			
		
				
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
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{

	if (Buffer->Memory)
	{
		//VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
		DeleteObject(Buffer->HBMP);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;

	int BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	
//	Buffer->HBMP = CreateDIBSection(DeviceContext, &Buffer->Info, DIB_RGB_COLORS, &Buffer->Memory, NULL,NULL);
	Buffer->Pitch = Width*BytesPerPixel;
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext,
	int WindowWidth, int WindowHeight,
	win32_offscreen_buffer Buffer)
{


	/*SelectObject(hMemDC, hDCBitmap);
	DeleteDC(hMemDC);*/
	StretchDIBits(DeviceContext,

		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer.Width, Buffer.Height,
		Buffer.Memory,
		&Buffer.Info,
		DIB_RGB_COLORS, SRCCOPY);

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
	case WM_SIZE:
	{
		Win32ResizeDIBSection(&GlobalBackBuffer, LOWORD(LParam), HIWORD(LParam));
		//ViewPort = viewport(0,0, LOWORD(LParam), HIWORD(LParam));
	} break;
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
			YOffset -=0.05f;
		if (keyCode == 'S')
			YOffset += 0.05f;

		if (keyCode == 'D')
			XOffset += 0.05f;
		if (keyCode == 'A')
			XOffset -= 0.05f;
	
		
	} break;

	case WM_MOUSEMOVE:
	{
		POINTS p = MAKEPOINTS(LParam);
		XMouse = p.x;
		YMouse = p.y;
		
	} break;

	case WM_PAINT:
	{
		Win32ClearBuffer(&GlobalBackBuffer, RGB(255,0,255));
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

Vec3f m2v(Matrix m) {
	return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}


Matrix v2m(Vec3f v) {
	Matrix m(4, 1);
	m[0][0] = v.x;
	m[1][0] = v.y;
	m[2][0] = v.z;
	m[3][0] = 1.f;
	return m;
}

Matrix viewport(int x, int y, int w, int h) {
	Matrix m = Matrix::identity(4);
	m[0][3] = x + w / 2.f;
	m[1][3] = y + h / 2.f;
	m[2][3] = 255.f / 2.f;

	m[0][0] = w / 2.f;
	m[1][1] = h / 2.f;
	m[2][2] = 255.f / 2.f;
	return m;
}

int CALLBACK
WinMain(HINSTANCE Instance, // a handle to our executable
	HINSTANCE PrevInstance,
	LPSTR CommandLine,
	int ShowCode)
{
	WNDCLASSW WindowClass = {};


	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = L"HandmadeHeroWindowClass";
	GlobalModel = new Model("SoftwareRender/obj/African_head.obj");
	
	if (RegisterClassW(&WindowClass))
	{
		HWND Window =
			CreateWindowExW(0,
				WindowClass.lpszClassName,
				L"SoftwareRender",
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				1280,
				720,
				0,
				0,
				Instance,
				0);
		if (Window)
		{
			HDC GlobalDeviceContext = GetDC(Window);

			Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
			win32_window_dimension Dimension = Win32GetWindowDimension(Window);

			int size = GlobalBackBuffer.Width * GlobalBackBuffer.Height;
			GlobalZBuffer = new float[size];
			glm::vec3 light_dir(0, 0, -1);
			
			
			
			glm::mat4 projection(1.0f);
			glm::mat4 view(1.0f);
			view = glm::lookAtLH(glm::vec3(0, 0, -2), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
			projection = glm::perspectiveFovLH(45.f, (float)GlobalBackBuffer.Width, (float)GlobalBackBuffer.Height, .1f, 100.f);
			glm::vec4 viewport(0, 0, GlobalBackBuffer.Width, GlobalBackBuffer.Height);

			GlobalRunning = true;
			while (GlobalRunning)
			{
				local_persist float lastX;
				local_persist float lastY;
				bool dirty = false;
				if (lastX != XOffset)
				{
					lastX = XOffset;
					dirty = true;
				}
				if (lastY != YOffset)
				{
					lastY = YOffset;
					dirty = true;
				}
				ScopedTimer time("Main-Loop");
				MSG Message;
				while (PeekMessageW(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}

					TranslateMessage(&Message);
					DispatchMessageW(&Message);
				}
				if (dirty)
				{
					Win32ClearBuffer(&GlobalBackBuffer, 0);
					for (int i = size; i--;)
					{

						GlobalZBuffer[i] = (std::numeric_limits<float>::max)();
					}

					//view = glm::lookAt(glm::vec3(0, 0, -XOffset), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
					for (int i = 0; i<GlobalModel->nfaces(); i++) {
						std::vector<int> face = GlobalModel->face(i);
						Vec3f screen_coords[3];
						glm::vec3 world_coords[3];

						for (int j = 0; j<3; j++) {

							Vec3f v = GlobalModel->vert(face[j]);
							glm::vec4 original(v.x, v.y, v.z, 1.0f);
							original = glm::rotate(XOffset, glm::vec3(0, 1, 0)) * original;
							original = glm::translate(glm::vec3(0, 0, YOffset + 10)) * original;

							int w = GlobalBackBuffer.Width;
							int h = GlobalBackBuffer.Height;

							glm::vec4 projected = projection * original;
							glm::vec4 ndc = projected / projected.w;
							ndc.x += 1.0f; ndc.y += 1.0f; ndc.z = ndc.z * 255;
							ndc.x *= 0.5f * (float)w;
							ndc.y *= 0.5f * (float)h;



							glm::vec4 winCoords = ndc;

							screen_coords[j] = Vec3f(winCoords.x, winCoords.y, winCoords.z);
							world_coords[j] = glm::vec3(original);
						}

						glm::vec3 n = glm::cross(world_coords[0] - world_coords[1], world_coords[0] - world_coords[2]);
						n = glm::normalize(n);
					
						float intensity = glm::dot(n, -light_dir);
						if (intensity < 0)
						{
							intensity = 1 - intensity;
							triangle(screen_coords, &GlobalBackBuffer, RGB((int)(255 * intensity), (int)(255 * intensity), (int)(255 * intensity)));
						}
						/*line(screen_coords[0].x, screen_coords[0].y, screen_coords[1].x, screen_coords[1].y, &GlobalBackBuffer, 0xff00ff);
						line(screen_coords[1].x, screen_coords[1].y, screen_coords[2].x, screen_coords[2].y, &GlobalBackBuffer, 0xff00ff);
						line(screen_coords[2].x, screen_coords[2].y, screen_coords[0].x, screen_coords[0].y, &GlobalBackBuffer, 0xff00ff);*/





					}
				}
				


				

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(GlobalDeviceContext,
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