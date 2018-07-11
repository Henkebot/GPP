#include <Windows.h>
#include <stdio.h>
#include <assert.h>
#include <vector>

int main()
{
	HANDLE ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	assert(ConsoleHandle != INVALID_HANDLE_VALUE);

	
	SetConsoleTextAttribute(ConsoleHandle, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	CONSOLE_CURSOR_INFO cciInfo;
	cciInfo.dwSize = 1;
	cciInfo.bVisible = false;
	SetConsoleCursorInfo(ConsoleHandle, &cciInfo);

	GetConsoleScreenBufferInfo(ConsoleHandle, &csbiInfo);


	


	std::vector<char> ScreenChars;
	ScreenChars.reserve(csbiInfo.srWindow.Right*csbiInfo.srWindow.Bottom);
	for (int i = csbiInfo.srWindow.Bottom; i--;)
	{
		
		for (int j = csbiInfo.srWindow.Right; j--;)
		{
			ScreenChars.push_back(' ');
		}
		ScreenChars.push_back('\n');
		
	}

	int lol = 0;
	int lol2 = 2;
	while (true)
	{
		SetConsoleTextAttribute(ConsoleHandle,BACKGROUND_BLUE);
		
		CONSOLE_SCREEN_BUFFER_INFO newInfo;
		GetConsoleScreenBufferInfo(ConsoleHandle, &newInfo);
		if (newInfo.srWindow.Right != csbiInfo.srWindow.Right)
		{
			csbiInfo.srWindow = newInfo.srWindow;
			ScreenChars.clear();
			ScreenChars.reserve(csbiInfo.srWindow.Right*csbiInfo.srWindow.Bottom);
			
		}
		ScreenChars.clear();
		for (int i = csbiInfo.srWindow.Bottom; i--;)
		{

			for (int j = csbiInfo.srWindow.Right; j--;)
			{
				ScreenChars.push_back(' ');
			}
			ScreenChars.push_back('\n');

		}


		COORD Coord = {0, 0};

		BOOL hr = SetConsoleCursorPosition(
			ConsoleHandle,
			Coord
		);

		DWORD Write = ScreenChars.size();
		DWORD Written;
	
		BOOL Result = WriteConsoleA(
			ConsoleHandle,
			(const VOID*)ScreenChars.data(),
			Write,
			&Written,
			NULL);
		assert(Result);

		SetConsoleTextAttribute(ConsoleHandle,BACKGROUND_GREEN);
		hr = SetConsoleCursorPosition(
			ConsoleHandle,
			{ 2,3}
		);
		Result = WriteConsoleA(
			ConsoleHandle,
			(const VOID*)ScreenChars.data(),
			Write / 2,
			&Written,
			NULL);

		assert(Result);

	}



	return(0);
}