#include <Windows.h>
#include <stdint.h>

typedef char* LPCHAR;

int main()
{
	HANDLE FileHandle = CreateFileA("C:/Cube.fbx",
								GENERIC_READ,
								0,
								0,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								0);
	if (SUCCEEDED(FileHandle))
	{
		OutputDebugStringA("File successfully opened!");
	}

	const int32_t MAX_SIZE = 1 << 27;
	DWORD lol = 20;
	LPCHAR FileBuffer = new char[MAX_SIZE];
	BOOL r = ReadFile(file,
				FileBuffer,
					)

	CloseHandle(file);
	return 0;
}