#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h> // For SetCaption()

class BaseWindow
{
protected:
	HWND m_hHandle;
	HINSTANCE m_hInstance;
	LPTSTR m_lpClassName;
	LPTSTR m_lpCaption;
	LPVOID m_lParam;

	BaseWindow();

public:
	BaseWindow(LPCTSTR _lpszCaption, LPWNDCLASSEX _lpwndClassEx);
	BaseWindow(LPCTSTR _lpszCaption, HINSTANCE _hInstance, LPCTSTR _lpszClassName);

	~BaseWindow();

	virtual VOID 
	SetCaption(LPCTSTR _lpszCaption);

	virtual VOID 
	SetClassName(LPCTSTR _lpszClassName);

	virtual HWND 
	Create();

	virtual HWND 
	Create(HWND hWndParent, DWORD dwExStyle, DWORD dwStyle,
		INT x, INT y, INT nWidth, INT nHeight, 
		HMENU hMenu, LPVOID lpParam);
	
	virtual BOOLEAN
	Show();

	virtual BOOLEAN
	Show(int nCmdShow);

private:
	virtual VOID _Register(LPWNDCLASSEX _lpwndClassEx);

	static LRESULT CALLBACK msgRouter(HWND, UINT, WPARAM, LPARAM);
	virtual LRESULT CALLBACK wndProc(UINT, WPARAM, LPARAM) = 0;
};