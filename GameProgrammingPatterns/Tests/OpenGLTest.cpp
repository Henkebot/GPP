#include "../OpenGL/TestWindow.h"

int CALLBACK
WinMain(HINSTANCE Instance, // a handle to our executable
	HINSTANCE PrevInstance,
	LPSTR CommandLine,
	int ShowCode)
{
	TestWindow* test = new TestWindow(TEXT("Test"), Instance, TEXT("TestWindow"));

	test->Create();
	test->Show();

	MSG  msg;
	int status;
	while ((status = GetMessage(&msg, 0, 0, 0)) != 0)
	{
		if (status == -1) {
			// handle the error, break
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}