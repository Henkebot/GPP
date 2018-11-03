#pragma once
#include "BaseWindow.h"

class TestWindow : public BaseWindow
{
public:
	TestWindow(LPCTSTR _lpszCaption, HINSTANCE _hInstance, LPCTSTR _lpszClassName);

	virtual LRESULT CALLBACK wndProc(UINT, WPARAM, LPARAM);
};