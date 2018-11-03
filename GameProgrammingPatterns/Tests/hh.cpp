#include <windows.h>
#include <stdint.h>
#include <fstream>
#include "../Timer/QPC.cpp"
#include "../SoftwareRender/Model.h"
#include <limits>
#include "../Other/TGAImage.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm.hpp>
#include <gtx\transform.hpp>
#include <gtc/matrix_transform.hpp>

#include "../SoftwareRender/Buffer.h"
#include "../SoftwareRender/My3D.h"

#define internal static
#define local_persist static
#define global_variable static

#define RGBA(r,g,b,a) (a << 24 | r << 16 | g << 8 | b)
#define RGB(r,g,b) RGBA(r,g,b,255)


struct win32_window_dimension
{
	int Width;
	int Height;
	int XPos;
	int YPos;
};

global_variable bool GlobalRunning;
//global_variable MY3D::RGBBuffer GlobalBackBuffer;
global_variable int XMouse;
global_variable int YMouse;
global_variable float W;
global_variable float S;
//global_variable MY3D::RGBBuffer GlobalZBuffer;
global_variable glm::mat4 lol;

global_variable bool KEY_W;
global_variable bool KEY_S;
global_variable bool KEY_D;
global_variable bool KEY_A;
global_variable bool KEY_F;

#include<gtc\quaternion.hpp>

class FPSCamera
{
private:
	float m_fPitch;
	float m_fYaw;
	glm::quat m_camera_quat;
	glm::mat4 m_viewMatrix;
	glm::vec2 m_MousePosition2f;
	glm::vec3 m_eyeVector;
public:
	FPSCamera() : m_fPitch(0), m_fYaw(0), m_MousePosition2f(0, 0), m_camera_quat(0, 0, 0, 0), m_eyeVector(0, 0, 0)
	{

	}
	glm::vec3 GetForwardDirection(glm::quat quat)
	{
		// Extract the vector part of the quaternion
		glm::vec3 u(quat.x, quat.y, quat.z);

		// Extract the scalar part of the quaternion
		float s = quat.w;
		glm::vec3 v(0, 0, -1);
		// Do the math
		return(2.0f * glm::dot(u, v) * u
			+ (s*s - dot(u, u)) * v
			+ 2.0f * s * cross(u, v));
	//	return quat * -glm::vec3(0, 0, -1) * glm::conjugate(quat);
	}

	glm::vec3 GetRightDirection(glm::quat quat)
	{	
		// Extract the vector part of the quaternion
		glm::vec3 u(quat.x, quat.y, quat.z);

		// Extract the scalar part of the quaternion
		float s = quat.w;
		glm::vec3 v(1, 0, 0);
		// Do the math
		return(2.0f * glm::dot(u, v) * u
			+ (s*s - dot(u, u)) * v
			+ 2.0f * s * cross(u, v));

	}
	glm::quat GetOrientation()
	{
		return glm::angleAxis(m_fYaw, glm::vec3(0, 1, 0)) * m_camera_quat * glm::angleAxis(m_fPitch, glm::vec3(1, 0, 0));
	}
	void UpdateView()
	{

		//temporary frame quaternion from pitch,yaw,roll 
		//here roll is not used
		
		glm::quat key_quat = GetOrientation();
		m_camera_quat = key_quat;
		m_camera_quat = glm::normalize(m_camera_quat);
	//	glm::quat key_quat = glm::angleAxis(m_fPitch, glm::vec3(1, 0, 0)) * glm::angleAxis(m_fYaw, glm::vec3(0, 1, 0));// glm::quat(glm::vec3(-m_fPitch, -m_fYaw, 0));
		//reset values

		glm::vec3 forward = GetForwardDirection(m_camera_quat);
		glm::vec3 right = GetRightDirection(m_camera_quat);
		if (KEY_W)
		{
			m_eyeVector -= forward * 0.01f;
		}
		else if (KEY_S)
		{
			m_eyeVector += forward * 0.01f;
		}

		if (KEY_A)
		{
			m_eyeVector -= right * 0.01f;
		}
		else if (KEY_D)
		{
			m_eyeVector += right * 0.01f;
		}


		m_fPitch = m_fYaw = 0;

		////order matters,update camera_quat
	
		glm::mat4 rotate = glm::mat4_cast(glm::conjugate(m_camera_quat));

		glm::mat4 translate = glm::mat4(1.0f);
		translate = glm::translate(translate, -m_eyeVector);

		m_viewMatrix = rotate * translate;

	}
	void MouseMove(int x, int y)
	{
		
		
		glm::vec2 currentMouse2f(x, y);
		glm::vec2 mouseDx = currentMouse2f - glm::vec2(718 - 86, 438 - 109); //m_MousePosition2f;
	
		const float mouseX_sens = 0.0005f;
		const float mouseY_sens = 0.0005f;
		m_fPitch = mouseY_sens * mouseDx.y;
		m_fYaw = mouseX_sens * mouseDx.x;
		UpdateView();
		m_MousePosition2f = currentMouse2f;
		
	}

	void EyeMove(glm::vec3 _offset)
	{
		
	
	}

	glm::mat4 getViewMat() const
	{
		return m_viewMatrix;
	}

};
//
//inline internal
//void Win32DrawLine(int x0, int y0, int x1, int y1, win32_offscreen_buffer* Buffer, int color)
//{
//	bool steep = false;
//	if (std::abs(x0 - x1) < std::abs(y0 - y1))
//	{
//		std::swap(x0, y0);
//		std::swap(x1, y1);
//		steep = true;
//	}
//
//	if (x0 > x1)
//	{
//		std::swap(x0, x1);
//		std::swap(y0, y1);
//	}
//	int dx = x1 - x0;
//	int dy = y1 - y0;
//	int derror2 = std::abs(dy) * 2;
//	int error2 = 0;
//	int y = y0;
//
//	for (float x = x0; x <= x1; x++)
//	{
//		if (steep)
//		{
//			Win32SetPixel(Buffer, y, x, color);
//		}
//		else
//		{
//			Win32SetPixel(Buffer, x, y, color);
//		}
//		error2 += derror2;
//		if (error2 > dx)
//		{
//			y += (y1 > y0) ? 1 : -1;
//			error2 -= dx * 2;
//		}
//	}
//}
//
//inline internal void
//triangle(glm::vec4 *pts, glm::vec2* uvs, glm::vec3* norms, MY3D::RGBBuffer* Buffer, float _color)
//{
//	glm::vec4 pts1[3];
//	for (int i = 0; i < 3; i++)
//	{
//		pts1[i] = pts[i] * lol;
//	}
//	glm::vec2 pts2[3];
//	glm::vec3 ptsForDepth[3];
//	// Pts2 are the projected points
//	for (int i = 0; i < 3; i++) pts2[i] = ptsForDepth[i] = (pts1[i] / pts1[i].w);
//
//
//	int Width = Buffer->GetWidth();
//	int Height = Buffer->GetHeight();
//	Vec2f bboxmin(Width, Height);
//	Vec2f bboxmax(-Width,-Height);
//	Vec2f clamp(Width, Height);
//	
//	for (int i = 3; i--;) {
//		for (int j = 2; j--;) {
//			bboxmin[j] = glm::max(0.f,		glm::min(bboxmin[j], pts2[i][j]));
//			bboxmax[j] = glm::min(clamp[j],	glm::max(bboxmax[j], pts2[i][j]));
//		}
//	}
//	glm::ivec2 P;
//	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
//		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
//
//			glm::vec3 bc_screen = barycentric(pts2, P);
//			
//			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
//				continue;
//
//			glm::vec3 bc_clip = glm::vec3(bc_screen.x / pts1[0].w, bc_screen.y / pts1[1].w, bc_screen.z / pts1[2].w);
//			bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
//			
//			float currentDepth = GlobalZBuffer.GetPixel(P.x, P.y);
//			if (currentDepth == -1)
//				continue;
//
//		
//			float fragDepth = 0;
//
//			fragDepth += ptsForDepth[0].z * bc_clip[0];
//			fragDepth += ptsForDepth[1].z * bc_clip[1];
//			fragDepth += ptsForDepth[2].z * bc_clip[2];
//			
//			if (currentDepth > fragDepth)
//				continue;
//
//
//
//			GlobalZBuffer.SetPixel(P.x, P.y, fragDepth);
//
//			glm::vec2 finalUv = uvs[0] * bc_clip.x + uvs[1] * bc_clip.y + uvs[2] * bc_clip.z;
//			TGA_Color color = GlobalModel->diffuse(Vec2i(finalUv.x, finalUv.y));
//			//color = TGA_Color(255, 255, 255, 255);
//			glm::vec3 finalNorm = norms[0] * bc_clip.x + norms[1] * bc_clip.y + norms[2] * bc_clip.z;
//
//			glm::vec3 lightDir(0, 0, 1);
//			float finalColor = glm::clamp(glm::dot(glm::normalize(finalNorm), lightDir), 0.f, 1.0f);
//
//
//			Buffer->SetPixel(P.x, P.y, RGB((int)(color.r * finalColor), (int)(color.g * finalColor), (int)(color.g * finalColor)));
//		
//			
//			
//
//				
//		}
//	}
//}

//inline internal void
//Plot4EllipsePoints(win32_offscreen_buffer* Buffer, Vec2i org, int X, int Y, int Color)
//{
//	Win32SetPixel(Buffer, org.x + X, org.y + Y, Color);
//	Win32SetPixel(Buffer, org.x - X, org.y + Y, Color);
//	Win32SetPixel(Buffer, org.x - X, org.y - Y, Color);
//	Win32SetPixel(Buffer, org.x + X, org.y - Y, Color);
//}
//
//internal void
//Win32DrawCircleQuick(win32_offscreen_buffer* Buffer, Vec2i vec, int radius, int color)
//{
//	
//	int x = radius - 1;
//	int y = 0;
//	int dx = 1;
//	int dy = 1;
//	int err = dx - (radius << 1);
//
//	while (x >= y)
//	{
//		Win32SetPixel(Buffer, vec.x + x, vec.y + y, color);
//		Win32SetPixel(Buffer, vec.x + y, vec.y + x, color);
//		Win32SetPixel(Buffer, vec.x - y, vec.y + x, color);
//		Win32SetPixel(Buffer, vec.x - x, vec.y + y, color);
//		Win32SetPixel(Buffer, vec.x - x, vec.y - y, color);
//		Win32SetPixel(Buffer, vec.x - y, vec.y - x, color);
//		Win32SetPixel(Buffer, vec.x + y, vec.y - x, color);
//		Win32SetPixel(Buffer, vec.x + x, vec.y - y, color);
//
//		if (err <= 0)
//		{
//			y++;
//			err += dy;
//			dy += 2;
//		}
//
//		if (err > 0)
//		{
//			x--;
//			dx += 2;
//			err += dx - (radius << 1);
//		}
//	}
//
//
//}
//
//internal void
//Win32DrawCircleTest(win32_offscreen_buffer* Buffer, Vec2i Position, int Radius, int Color)
//{
//	int x = 0;
//	int y = Radius;
//	int p = 3 - 2 * Radius;
//	int xc = Position.x;
//	int yc = Position.y;
//
//	while (y >= x) // only formulate 1/8 of circle
//	{
//		Win32SetPixel(Buffer, xc - x, yc - y,Color);//upper left left
//		Win32SetPixel(Buffer, xc - y, yc - x,Color);//upper upper left
//		Win32SetPixel(Buffer, xc + y, yc - x,Color);//upper upper right
//		Win32SetPixel(Buffer, xc + x, yc - y,Color);//upper right right
//		Win32SetPixel(Buffer, xc - x, yc + y,Color);//lower left left
//		Win32SetPixel(Buffer, xc - y, yc + x,Color);//lower lower left
//		Win32SetPixel(Buffer, xc + y, yc + x,Color);//lower lower right
//		Win32SetPixel(Buffer, xc + x, yc + y,Color);//lower right right
//		if (p < 0) p += 4 * x++ + 6;
//		else p += 4 * (x++ - y--) + 10;
//	}
//}
//
//internal void
//Win32DrawCircleFill(win32_offscreen_buffer* Buffer, Vec2i Position, int Radius, int Color)
//{
//	// Taken from wikipedia
//	int x = 0;
//	int y = Radius;
//	int p = 3 - 2 * Radius;
//	int xc = Position.x;
//	int yc = Position.y;
//
//	auto drawline = [&](int sx, int ex, int ny)
//	{
//		for (int i = sx; i <= ex; i++)
//			Win32SetPixel(Buffer, i, ny,Color);
//	};
//
//	while (y >= x)
//	{
//		// Modified to draw scan-lines instead of edges
//		drawline(xc - x, xc + x, yc - y);
//		drawline(xc - y, xc + y, yc - x);
//		drawline(xc - x, xc + x, yc + y);
//		drawline(xc - y, xc + y, yc + x);
//		if (p < 0) p += 4 * x++ + 6;
//		else p += 4 * (x++ - y--) + 10;
//	}
//}
//
//internal void
//Win32DrawCircle(win32_offscreen_buffer* Buffer, Vec2i vec, int radius, int color)
//{
//	int startX = vec.x - radius; int endX = startX + (radius << 1);
//	int startY = vec.y - radius; int endY = startY + (radius << 1);
//	int LookX = 0;
//	int LookY = 0;
//	int LookZ = 1;
//	int radiusSqrt = radius * radius;
//	
//	Vec2i cur;
//
//
//	for (cur.x = startX; cur.x < endX; cur.x++)
//	{
//		for (cur.y = startY; cur.y < endY; cur.y++)
//		{
//			Vec2i diff = cur - vec;
//
//			int length = (diff.y*diff.y) + (diff.x* diff.x);
//
//			if (length < radiusSqrt)
//			{
//				int c = color;
//				
//				float distance = 1 - (float(length) / radiusSqrt);
//				int finalC = 128 * distance;
//				c = RGB(finalC, finalC, finalC);			
//
//				Win32SetPixel(Buffer, cur.x, cur.y, c);
//			}
//		}
//	}
//
//}
//
//internal void
//Win32DrawRect(win32_offscreen_buffer* Buffer, RECT rect, int color)
//{
//	int xPos = rect.left;
//	int yPos = rect.top;
//
//	int width = rect.right - rect.left;
//	int height = rect.bottom - rect.top;
//
//	for (int X = 0; X < width; ++X)
//	{
//		for (int Y = 0; Y < height; ++Y)
//		{
//			Win32SetPixel(Buffer, X + xPos, Y + yPos, color);
//		}
//	}
//
//}


win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetWindowRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	Result.XPos = ClientRect.left;
	Result.YPos = ClientRect.top;

	return (Result);
}

internal void
Win32DisplayRGBBufferInWindow(HDC DeviceContext,
	int WindowWidth, int WindowHeight,
	MY3D::RGBBuffer* Buffer)
{

	StretchDIBits(DeviceContext,

		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer->GetWidth(), Buffer->GetHeight(),
		Buffer->GetBufferData(),
		Buffer->GetBitmapInfo(),
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
		/*GlobalBackBuffer.Resize(LOWORD(LParam), HIWORD(LParam));
		GlobalZBuffer.Resize(LOWORD(LParam), HIWORD(LParam));*/

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

		if (keyCode == VK_ESCAPE)
			GlobalRunning = FALSE;

		if (keyCode == 'W')
			KEY_W = keyDown;
		else if (keyCode == 'S')
			KEY_S = keyDown;

		if (keyCode == 'A')
			KEY_A = keyDown;
		else if (keyCode == 'D')
			KEY_D = keyDown;

		if (keyCode == 'F')
			KEY_F = keyDown;
	
		
	} break;

	case WM_MOUSEMOVE:
	{
		POINTS p = MAKEPOINTS(LParam);
		XMouse = p.x;
		YMouse = p.y;
		win32_window_dimension dim  = Win32GetWindowDimension(Window);
		int xValue = dim.XPos + (dim.Width >> 1);
		int yValue = dim.YPos + (dim.Height >> 1);
		SetCursorPos(xValue, yValue);
		
	} break;

	case WM_PAINT:
	{
		//GlobalBackBuffer.Clear(RGB(255, 0, 255));
		
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Window, &Paint);

		win32_window_dimension Dimension = Win32GetWindowDimension(Window);

	/*	Win32DisplayRGBBufferInWindow(DeviceContext,
			Dimension.Width,
			Dimension.Height,
			&GlobalBackBuffer);*/
		EndPaint(Window, &Paint);
	} break;

	default:
	{
		Result = DefWindowProc(Window, Message, WParam, LParam);
	} break;
	}

	return(Result);
}

class MyShader : public MY3D::SHADER
{
private:

	glm::mat4 rotation = glm::rotate(2.f * 3.1415f, glm::vec3(1, 0, 0));
	glm::vec3 lightdir = glm::vec3(1.f,0.f,1.f);
	glm::vec3 viewdir = glm::vec3(0.f, 0.f, -1.0f);
public:
	

	glm::vec4 Vertex(int vertexID)
	{
		glm::vec4 v{ m_data->vert(vertexID),1.0f };
		
		glm::vec4 projected = m_mat4["projview"]  * rotation * v;
		return projected;
	}

	glm::vec4 Fragment(int face, glm::vec3 inter)
	{
		//inter = glm::vec3(0.3, 0.3, 0.3);

		glm::ivec2 uv = m_data->uv(face, 0) * inter[0] +
						m_data->uv(face, 1) * inter[1] +
						m_data->uv(face, 2) * inter[2];
			
		
		glm::vec3 norms= m_data->norm(face,0) * inter[0] +
						m_data->norm(face, 1) * inter[1] +
						m_data->norm(face, 2)* inter[2];
		
		const float kPi = 3.14159264f;
		const float kShininess = 16.0f;

		norms = glm::normalize(rotation * glm::vec4(norms, 1.0f));


		glm::vec4 ambient(255.f*0.2f, 255.f*0.2f, 255.f*0.2f, 255.0f);

		TGA_Color color = m_data->diffuse(uv);//TGA_Color(255.f* 0.247059f, 255.f * 0.282353f, 255.f * 0.8f, 255)
		float finalColor = glm::clamp(glm::dot(norms, lightdir), 0.0f, 1.0f);
		glm::vec4 diffuse = glm::vec4(finalColor * color.b, finalColor * color.g, finalColor * color.r, 255.0f);

		const float kEnergyConservation = (8.f + kShininess) / (8.0f*kPi);
		glm::vec3 halfwayDir = glm::normalize(lightdir + viewdir);
		float spec = kEnergyConservation * glm::pow(glm::max(glm::dot(norms, halfwayDir), 0.0f), kShininess);

		glm::vec4 specular(spec, spec, spec, 255.f);
		

		return glm::clamp(diffuse + specular,0.f,255.f);
	}
};


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
	WindowClass.lpszClassName = L"SoftwareRender";
	
	
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

			
			const win32_window_dimension res[] =
			{
				{256,144},
				{426,240},
				{1280, 720},
				{1920,1080}
			};
			
			win32_window_dimension Dimension = res[1]; //Win32GetWindowDimension(Window);

			MY3D::RenderDevice rd;
			rd.SetBufferSize(Dimension.Width, Dimension.Height);

			FPSCamera cam;
			
			glm::mat4 projection(1.0f);

			MY3D::VIEWPORT vp;
			vp.Width = Dimension.Width;
			vp.Height = Dimension.Height;
			vp.Far = 512.f;
			vp.Near = 1.f;
			vp.X = 0;
			vp.Y = 0;
			rd.SetViewport(vp);
			glm::vec2 dim = rd.GetBufferSize();
			projection = glm::perspectiveFovLH(45.f, (float)dim.x, (float)dim.y, 1.f, 255.f);

			MY3D::SHADER* myShader = new MyShader();
			myShader->m_data = new Model("SoftwareRender/obj/african_head.obj");
			
			//myShader->setUniform4x4f("proj", projection);

			rd.SetShader(myShader);
			GlobalRunning = true;
			while (GlobalRunning)
			{
				

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
				
		
				cam.MouseMove(XMouse, YMouse);

				myShader->setUniform4x4f("projview", projection * cam.getViewMat());

				rd.Clear(DEPTH_BUFFER);

				rd.Render();

					
				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayRGBBufferInWindow(GlobalDeviceContext,
					Dimension.Width,
					Dimension.Height,
					rd.GetFrontBuffer());

			}
		}
		else
		{
			// TODO(Henrik) Logging
		}
	}
	else
	{

		// TODO(Henrik): Logging
	}

	return (0);
}