#include "BaseWindow.h"
#include <stdlib.h> // for calloc
#include <assert.h>


static VOID SafeReplaceLPCTSTR(LPTSTR& dst, LPCTSTR src)
{
	int nLen;
	if (dst != nullptr)
	{
		delete dst;
	}

	nLen = _tcsnlen(src, 255) + 1;
	dst = (LPTSTR)calloc(nLen, sizeof(TCHAR));
	_tcscpy_s(dst, nLen, src);
}

static VOID SafeFreeLPCTSTR(LPTSTR* _src)
{
	if(*_src != nullptr)
	{
		delete[] *_src;
		*_src = nullptr;
	}
}

BaseWindow::BaseWindow()
{
	m_hHandle = nullptr;
	m_hInstance = nullptr;
	m_lpClassName = nullptr;
	m_lpCaption = nullptr;
}

BaseWindow::BaseWindow(LPCTSTR _lpszCaption, LPWNDCLASSEX _lpwndClassEx)
{
	SetCaption(_lpszCaption);
	SetClassName(_lpwndClassEx->lpszClassName);
	m_hInstance = _lpwndClassEx->hInstance;

}

BaseWindow::BaseWindow(LPCTSTR _lpszCaption, HINSTANCE _hInstance, LPCTSTR _lpszClassName)
	: m_hHandle(0), m_lpCaption(0), m_lpClassName(0)
{
	WNDCLASSEX wnd;

	SetCaption(_lpszCaption);
	SetClassName(_lpszClassName);
	m_hInstance = _hInstance;
	
	wnd.cbSize = sizeof(WNDCLASSEX);
	wnd.cbClsExtra = 0;
	wnd.cbWndExtra = 0;
	wnd.style = CS_HREDRAW | CS_VREDRAW;
	wnd.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wnd.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wnd.lpszMenuName = NULL;

	_Register(&wnd);

}

BaseWindow::~BaseWindow()
{
	if(m_hHandle != nullptr)
	{
		DestroyWindow(m_hHandle);
		m_hHandle = nullptr;
	}
	SafeFreeLPCTSTR(&m_lpClassName);
	SafeFreeLPCTSTR(&m_lpCaption);
}

VOID BaseWindow::SetCaption(LPCTSTR  _lpszCaption)
{
	if(_lpszCaption != nullptr && m_hHandle == nullptr)
	{
		SafeReplaceLPCTSTR(m_lpCaption, _lpszCaption);
	}
	// NOTE(Henrik): Logging
}

VOID BaseWindow::SetClassName(LPCTSTR _lpszClassName)
{
	if (_lpszClassName != nullptr && m_hHandle == nullptr)
	{
		SafeReplaceLPCTSTR(m_lpClassName, _lpszClassName);
	}
	// NOTE(Henrik): Logging
	
}

HWND BaseWindow::Create()
{
	return Create((HWND) NULL,
		0L,WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		(HMENU) NULL,
		NULL);
}

HWND BaseWindow::Create(HWND hWndParent, DWORD dwExStyle, DWORD dwStyle, INT x, INT y, INT nWidth, INT nHeight, HMENU hMenu, LPVOID lpParam)
{
	if(m_hHandle == nullptr)
	{
		m_hHandle = CreateWindowEx(dwExStyle, m_lpClassName, m_lpCaption, dwStyle, x, y, nWidth, nHeight, (HWND)hWndParent, (HMENU)hMenu, m_hInstance, this);
		if (m_hHandle == NULL)
		{
			//NOTE(Henrik) Logging
		}

		m_lParam = lpParam;
	}

	return m_hHandle;
}

BOOLEAN BaseWindow::Show()
{
	return(Show(SW_SHOW));
}

BOOLEAN BaseWindow::Show(int nCmdShow)
{
	return (this->m_hHandle == nullptr) ? FALSE : ShowWindow(m_hHandle, nCmdShow);
}

VOID BaseWindow::_Register(LPWNDCLASSEX _lpwndClassEx)
{
	assert(_lpwndClassEx);

	_lpwndClassEx->lpfnWndProc = msgRouter;
	_lpwndClassEx->lpszClassName = m_lpClassName;
	_lpwndClassEx->hInstance = m_hInstance;

	if (!RegisterClassEx(_lpwndClassEx))
	{
		//NOTE(Henrik): Logging
	}
	
}

LRESULT BaseWindow::msgRouter(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
{
	BaseWindow* wnd = nullptr;

	if (_uMsg != WM_CREATE)
	{
		wnd = (BaseWindow*)GetWindowLongPtr(_hWnd, GWLP_USERDATA);
		if (!wnd) return DefWindowProc(_hWnd, _uMsg, _wParam, _lParam);
		
	}
	else
	{
		wnd = (BaseWindow*)(LPCREATESTRUCT(_lParam)->lpCreateParams);
		SetWindowLongPtr(_hWnd, GWLP_USERDATA, (LONG_PTR)wnd);
	}

	return wnd->wndProc(_uMsg, _wParam, _lParam);

}
